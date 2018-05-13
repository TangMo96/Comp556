#include "server.h"


int create_sock(uint16_t port) {
	/* socket variable */
	struct sockaddr_in sin;

	/* for socket option */
	int opt_val = 1;

	/* the created socket */
	int sock;

	/* create a server socket to listen for TCP connection requests */
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror("error opening TCP socket\n");
		abort();
	}

	/* set option so we can reuse the port number quickly after a restart */
	if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt_val, sizeof (opt_val)) <0) {
		perror ("error setting TCP socket option\n");
		abort();
	}

	/* fill in the address of the server socket */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons(port);

	/* bind server socket to the address */
	if (bind(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("error binding socket to address\n");
		abort();
	}

	/* put the server socket in listen mode */
	if (listen(sock, MAX_CONNECTION) < 0) {
		perror("listen on socket failed\n");
		abort();
	}

	return sock;
}

int select_fds(int sock, fd_set *read_set, fd_set *write_set, struct node *head) {
	/* variables for select */
	int max_fd, select_ret_val;
	struct node *current;
	struct timeval time_out;

	/* clear fd set */
	FD_ZERO(read_set);
	FD_ZERO(write_set);

	/* put the listening socket in */
	FD_SET(sock, read_set);

	/* initialize max_fd */
	max_fd = sock;

	/* put connected sockets into the read and write sets to monitor them */
	for (current = head->next; current; current = current->next) {
		FD_SET(current->fd, read_set);

		if (current->operation_type == IS_SENDING) FD_SET(current->fd, write_set);

		/* update max if necessary */
		if (current->fd > max_fd) max_fd = current->fd;
	}

	/* set timeout */
	time_out.tv_usec = TIMEOUT_TIME;
	time_out.tv_sec = 0;

	/* invoke select */
	select_ret_val = select(max_fd + 1, read_set, write_set, NULL, &time_out);
	if (select_ret_val < 0) {
		perror("select failed\n");
		abort();
	}

	return select_ret_val;
}

int create_fd(int sock, struct node *head) {
	/* socket variable */
	struct sockaddr_in addr;
	socklen_t addr_len = sizeof(struct sockaddr_in);

	/* the created fd */
	int new_fd;

	new_fd = accept(sock, (struct sockaddr *) &addr, &addr_len);

	if (new_fd < 0) {
		perror("error accepting connection\n");
		abort();
	}

	/* make the socket non-blocking */
	if (fcntl (new_fd, F_SETFL, O_NONBLOCK) < 0) {
		perror("error making socket non-blocking\n");
		abort();
	}

	/* print client info */
	if (DEBUG) printf("fd#%d: accepted; IP: %s\n", new_fd, inet_ntoa(addr.sin_addr));

	/* remember this client connection */
	add(head, new_fd, addr);

	return new_fd;
}

void send_message(struct node *current) {
	/* number of bytes sent */
	ssize_t count;

	count = send(current->fd, current->message + current->processed_message,
				 current->pending_message, MSG_DONTWAIT);

	if (count < 0 && errno != EAGAIN) {
		perror("error sending message to a client\n");
		abort();
	} else {
		current->processed_message += count;
		current->pending_message -= count;

		if (DEBUG)
			printf("fd#%d: current sent: %ld/%ld\n",
				   current->fd, current->processed_message,
				   current->processed_message + current->pending_message);
	}
}

int recv_message(struct node *head, struct node *current, char *recv_buf, int mode) {
	/* the length of a message */
	size_t message_size;

	/* number of bytes received */
	ssize_t count;

	count = recv(current->fd, recv_buf, MAX_MESSAGE_LENGTH, 0);

	/* whether there's an error */
	if (count < 2) {
		/* connection is closed, clean up */
		if (count == 0) {
			if (DEBUG) printf("fd#%d: closed; IP: %s\n",
							  current->fd, inet_ntoa(current->client_addr.sin_addr));

			close(current->fd);
			dump(head, current->fd);

			return 0;
		} else if (mode == MODE_WEB_SERVER) {
			return -1;
		} else {
			perror("error receiving from a client\n");
			abort();
		}
	} else {
		/* if receiving data for the first time, handle specially */
		if (!current->processed_message) {
			if (DEBUG) printf("fd#%d: begin to receive\n", current->fd);

			message_size = mode == MODE_PING_PONG ?
						   (size_t) ntohs(*(uint16_t *) recv_buf) : MAX_MESSAGE_LENGTH;

			/* free old memory */
			if (current->message != NULL) {
				free(current->message);
				current->message = NULL;
			}

			/* allocate memory */
			current->message = (char *) malloc(message_size);
			if (!current->message) {
				if (mode == MODE_WEB_SERVER) {
					return -1;
				} else {
					perror("failed to allocated node.message\n");
					abort();
				}
			}

			memset(current->message, '\0', message_size);

			current->pending_message = message_size;
		}

		/* check limit */
		if (count > current->pending_message || current->processed_message + count > MAX_MESSAGE_LENGTH) {
			if (mode == MODE_WEB_SERVER) {
				return -1;
			} else {
				perror("too much message\n");
				abort();
			}
		}

		/* save data */
		memcpy(current->message + current->processed_message, recv_buf, (size_t) count);
		current->processed_message += count;
		if (mode == MODE_PING_PONG) current->pending_message -= count;

		if (DEBUG) printf("fd#%d: current received: %ld/%ld\n",
						  current->fd, current->processed_message,
						  current->processed_message + current-> pending_message);

		return 1;
	}
}

int extract_file_path(char *message, size_t m_length, char *file_path) {
	int i, c, l;

	if (strncmp(message, "GET ", 4) != 0) return 0;

	// calculate the length
	for (c = 4; c < m_length && message[c] != ' '; c++);

	// check '../'
	for (i = 4; i + 3 <= c; i++)
		if (strncmp(message + i, "../", 3) == 0) return -1;

	if (strncmp(message + c + 1, "HTTP/1.1", 8) != 0) return 0;

	l =  c - 4;
	memcpy(file_path, message + 4, (size_t) l);

	return l;
}

int read_file(char *path, char *res) {
	FILE *file = fopen(path,"r");

	/* get the length of the file */
	fseek(file, 0, SEEK_END);
	long len = ftell(file);

	/* begin to read file */
	rewind(file);
	fread(res, 1, (size_t) len, file);

	fclose(file);

	return (int) len;
}

int check_existence_and_folder(char *path) {
	/* check existence */
	if( (access(path, 0)) == -1) return -1;

	/* check folder */
	struct stat buf;
	stat(path, &buf);

	if(S_IFDIR & buf.st_mode) return 0;
	else return 1;
}

void generate_header(struct node *current, char *title) {
	char *response_header= "Content-Type: text/html\r\n\r\n";

	memcpy(current->message, title, strlen(title));
	current->pending_message += strlen(title);

	memcpy(current->message + current->pending_message,
		   response_header, strlen(response_header));
	current->pending_message += strlen(response_header);
}
