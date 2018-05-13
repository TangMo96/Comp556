#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <limits.h>
#include <sys/time.h>
#include <errno.h>
#include <netdb.h>


#define DEBUG 0


int main(int argc, char** argv) {

	/* fd of the client socket */
	int sock;

	/* address structure for identifying the server */
	struct sockaddr_in sin;

	/* time structure for system time */
	struct timeval tv;

	/* for loop */
	int i;

	/* average latency */
	double avg_lat;

	/* convert server domain name to IP address */
	struct hostent *host = gethostbyname(argv[1]);
	if (!host) {
		printf("invalid server name or ip\n");
		abort();
	}

	/* server ip address */
	uint32_t server_addr = *(uint32_t *) host->h_addr_list[0];

	/* the port on which the server is running */
	long server_port = strtol(argv[2], NULL, 0);
	if (!server_port
		|| server_port == LONG_MAX || server_port == LONG_MIN
		|| server_port < 18000 || server_port > 18200) {
		printf("invalid server port number: [18000, 18200])\n");
		abort();
	}

	/* the size in bytes of each message to send */
	size_t total_size = (size_t) strtol(argv[3], NULL, 0);
	if (!total_size
		|| total_size == LONG_MAX || total_size == LONG_MIN
		|| total_size < 10 || total_size > 65535) {
		printf("invalid data size: [10, 65535]\n");
		abort();
	}

	/* the number of message exchanges to perform */
	long exchange_count = strtol(argv[4], NULL, 0);
	if (!exchange_count
		|| exchange_count == LONG_MAX || exchange_count == LONG_MIN
		|| exchange_count > 10000) {
		printf("invalid data size: [1, 10000]\n");
		abort();
	}

	/* the length of the sent/received message */
	ssize_t count;

	/* the length of the sent/received message in one round */
	ssize_t curr_count;

	/* allocate space for buffers */
	char *send_buf = (char *) malloc((size_t) total_size);
	if (!send_buf) {
		perror("failed to allocate send_buf\n");
		abort();
	}

	char *recv_buf = (char *) malloc((size_t) total_size);
	if (!recv_buf) {
		perror("failed to allocate recv_buf\n");
		abort();
	}

	/* create a socket */
	if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		perror ("opening TCP socket\n");
		abort ();
	}

	/* fill in the server's address */
	memset(&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = server_addr;
	sin.sin_port = htons((uint16_t) server_port);

	/* connect to the server */
	if (connect(sock, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("connect to server failed\n");
		abort();
	}

	/* init message */
	memset(send_buf, 'c', total_size);

	if (DEBUG) send_buf[55555] = 't';

	/* set size */
	*(uint16_t *) send_buf = htons((uint16_t) total_size);

	/* set timestamp */
	gettimeofday(&tv, NULL);
	if (DEBUG) printf("start time: %ld, %ld\n", tv.tv_sec, tv.tv_usec);

	*(uint32_t *) (send_buf + 2) = (uint32_t) htonl((uint32_t) tv.tv_sec);
	*(uint32_t *) (send_buf + 6) = (uint32_t) htonl((uint32_t) tv.tv_usec);

	/* send message */
	for (i = 0; i < exchange_count; i++) {
		count = 0;
		while (count < total_size) {
			curr_count = send(sock, send_buf + count, total_size - count, 0);
			if (curr_count < 0 && errno != EAGAIN) {
				perror("error sending to server\n");
				abort();
			}

			count += curr_count;

			if (DEBUG) printf("loop#%d: sending %ld/%ld\n", i + 1, count, total_size);
		}

		count = 0;
		while (count < total_size) {
			curr_count = recv(sock, recv_buf + count, total_size - count, 0);
			if (curr_count < 0) {
				perror("error receiving from server\n");
				abort();
			}

			count += curr_count;

			if (DEBUG) printf("loop#%d: receiving %ld/%ld\n", i + 1, count, total_size);
		}

		if (DEBUG) printf("loop#%d: test#55555: %c\n", i + 1, recv_buf[55555]);
	}

	/* extract timestamp */
	tv.tv_sec = (uint32_t) ntohl(*(uint32_t *) (recv_buf + 2));
	tv.tv_usec = (uint32_t) ntohl(*(uint32_t *) (recv_buf + 6));

	if (DEBUG) printf("extracted time: %ld, %ld\n", tv.tv_sec, tv.tv_usec);

	avg_lat = tv.tv_sec * 1000000.0 + tv.tv_usec;

	/* get new timestamp */
	gettimeofday(&tv, NULL);
	if (DEBUG) printf("end time: %ld, %ld\n", tv.tv_sec, tv.tv_usec);

	/* calculate latency */
	avg_lat = tv.tv_sec * 1000000.0 + tv.tv_usec - avg_lat;

	printf("%.3f\n", avg_lat / exchange_count / 1000);

}