#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

#include "node.h"
#include "server.h"


int main(int argc, char **argv) {

	/* the mode of the server */
	int mode;

	/* variable in web server mode */
	int res, flag;

	/* base dir of the web server */
	char *base_dir;

	/* the path of the target file */
	char *path;

	/* the length of the file_path */
	int count;

	/* used in response*/
	char *response_200 = "HTTP/1.1 200 OK\r\n";
	char *response_400 = "HTTP/1.1 400 Bad Request\r\n";
	char *response_404 = "HTTP/1.1 404 Not Found\r\n";
	char *response_500 = "HTTP/1.1 500 Internal Server Error\r\n";
	char *response_501 = "HTTP/1.1 501 Not Implemented\r\n";

	if (argc <= 2 || strncmp(argv[2], "www", 3) != 0) {
		mode = MODE_PING_PONG;
	} else {
		mode = MODE_WEB_SERVER;
		base_dir = argv[3];
	}

	/* socket variables */
	int sock;

	/* the port on which the server is running */
	long server_port = strtol(argv[1], NULL, 0);
	if (server_port <= 0
		|| server_port == LONG_MAX || server_port == LONG_MIN
		|| server_port < 18000 || server_port > 18200) {
		printf("invalid server port number: [18000, 18200])\n");
		abort();
	}

	/* linked list for keeping track of connected sockets */
	struct node head;
	struct node *current, *next;

	/* a buffer to read data */
	char *recv_buf = (char *) malloc(MAX_MESSAGE_LENGTH);
	if (!recv_buf) {
		perror("failed to allocate recv_buf\n");
		abort();
	}

	/* initialize head node of linked list */
	head.fd = -1;
	head.next = 0;

	/* variables for select */
	int select_ret_val;
	fd_set read_set, write_set;

	sock = create_sock((uint16_t) server_port);

	/* keep waiting for incoming connections */
	while (1) {
		select_ret_val = select_fds(sock, &read_set, &write_set, &head);

		/* no descriptor ready or timeout happened */
		if (select_ret_val == 0) continue;

		/* at least one file descriptor is ready */
		if (select_ret_val > 0) {
			/* check the server socket */
			if (FD_ISSET(sock, &read_set)) {
				create_fd(sock, &head);
			}

			/* check other fds, see if there is anything to read/send */
			for (current = head.next; current; current = next) {
				next = current->next;

				/* see if we have any message to send/receive */
				if (FD_ISSET(current->fd, &write_set)) {
					send_message(current);

					/* have sent all data */
					if (current->pending_message == 0) {
						if (DEBUG) printf("fd#%d: all sent\n", current->fd);

						if (current->message != NULL) {
							free(current->message);
							current->message = NULL;
						}

						// update values
						current->operation_type = IS_RECEIVING;
						current->processed_message = 0;

						// if the mode is web serve, close the fd
						if (mode == MODE_WEB_SERVER) {
							if (DEBUG) printf("fd#%d: closed manually\n", current->fd);

							close(current->fd);
							dump(&head, current->fd);
						}
					}
				} else if (FD_ISSET(current->fd, &read_set)) {
					flag = recv_message(&head, current, recv_buf, mode);
					/* send error message */
					if (flag < 0 && mode == MODE_WEB_SERVER) {
						generate_header(current, response_500);

						current->operation_type = IS_SENDING;
						current->processed_message = 0;

						/****************/
						abort();
						/****************/
					} else if (flag) {
						/* if the current mode is web server, handle specially */
						if (mode == MODE_WEB_SERVER) {
							/* check whether the message ends */
							if (strncmp(current->message + current->processed_message - 4,
										"\r\n\r\n", 4) == 0) {
								current->pending_message = 0;
							}
						}

						/* have received all data */
						if (!current->pending_message) {
							if (DEBUG) printf("fd#%d: all received\n", current->fd);

							/* if the current mode is web server, handle specially */
							if (mode == MODE_WEB_SERVER) {
								count = extract_file_path(current->message, current->processed_message, recv_buf);
								if (count == 0) {
									generate_header(current, response_501);

									if (DEBUG) printf("fd#%d: error 501\n", current->fd);
								} else if (count < 0) {
									generate_header(current, response_400);

									if (DEBUG) printf("fd#%d: error 400\n", current->fd);
								} else {
									/* generate the file path */
									path = (char *) malloc(strlen(base_dir) + count + 1);
									memcpy(path, base_dir, strlen(base_dir));

									memcpy(path + strlen(base_dir), recv_buf, (size_t) count);
									path[strlen(base_dir) + count] = '\0';

									if (DEBUG) printf("fd#%d: required path: %s\n", current->fd, path);

									/* generate message */
									res = check_existence_and_folder(path);
									if (res <= 0) {
										generate_header(current, response_404);

										if (DEBUG) printf("fd#%d: error 404\n", current->fd);
									} else {
										generate_header(current, response_200);

										count = read_file(path, current->message + current->pending_message);
										current->pending_message += count;

										memcpy(current->message + current->pending_message, "\r\n\r\n", 4);
										current->pending_message += 4;
									}
								}
							}

							// if the mode is ping-pong, send back to client
							if (mode == MODE_PING_PONG) {
								if (DEBUG) printf("fd#%d: begin to send\n", current->fd);

								current->pending_message = current->processed_message;
							}

							current->operation_type = IS_SENDING;
							current->processed_message = 0;
						}
					}
				}
			}
		}
	}
}
