#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <sys/time.h>
#include <fcntl.h>

#include "Runner.h"


Runner::Runner(char *ip, uint16_t port, char *fp) {
	init(ip, port, fp);

	config_socket();
}

// init all values
void Runner::init(char *ip, uint16_t port, char *fp) {
	// init local address info
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = INADDR_ANY;
	local_addr.sin_port = htons((uint16_t) LOCAL_PORT_NUMBER);

	// init receiver address info
	memset(&receiver_addr, 0, sizeof(receiver_addr));
	receiver_addr.sin_family = AF_INET;
	receiver_addr.sin_addr.s_addr = inet_addr(ip);
	receiver_addr.sin_port = htons((uint16_t) port);

	// init buff
	buff = new char[BUFF_LENGTH];

	// init handler
	handler.init(fp);
}

// create and config the socket
void Runner::config_socket() {
	// create the socket
	socket_fd = socket(PF_INET, SOCK_DGRAM, 0);                               /* TO-DO handle error */

	// bind the socket to local address
	bind(socket_fd, (sockaddr *) &local_addr, sizeof(sockaddr));              /* TO-DO handle error */

	// make the socket non-block
	fcntl(socket_fd, F_SETFL, O_NONBLOCK);                                    /* TO-DO handle error */
}

// begin
void Runner::start() {
	// for timer
	timeval tv;
	time_t current_time;
	time_t last_check_time;

	// for select
	fd_set read_set;
	timeval select_timeout_tv;
	select_timeout_tv.tv_usec = SELECT_TIMEOUT_TIME;
	select_timeout_tv.tv_sec = 0;

	// for socket
	packet_info *pi;
	ssize_t recv_len;
	uint16_t ack;
	uint16_t acc_ack;
	int all_sent_retry_count = 0;

	// start timer
	gettimeofday(&tv, nullptr);
	last_check_time = tv.tv_sec * 1000000 + tv.tv_usec;

	// begin to listen
	while (true) {
		// get current time
		gettimeofday(&tv, nullptr);
		current_time = tv.tv_sec * 1000000 + tv.tv_usec;

		// check interval
		if ((current_time - last_check_time) > PACKET_TIMEOUT_CHECKING_CYCLE) {
			auto vec = handler.get_not_acked_timeout_packets(current_time);

			// auto exit after some retries (if the finish_ack is lost)
			if (!vec->empty() && handler.is_all_sent() && ++all_sent_retry_count > MAX_RETRY_COUNT_AFTER_ALL_SENT) {
				printf("[completed] exit after max retries\n");

				break;
			}

			// resend timeout packets
			for (size_t i = 0; i < vec->size(); i++) {
				pi = (*vec)[i];

				// reset send time
				gettimeofday(&tv, nullptr);
				current_time = tv.tv_sec * 1000000 + tv.tv_usec;
				pi->send_time = current_time;

				if (pi->seq == 0) printf("[send data] TIMEOUT / (%d)\n", pi->packet_len);
				else printf("[send data] TIMEOUT %d (%d)\n", (pi->seq - 1) * DATA_PACKET_DATA_LENGTH, pi->packet_len);

				sendto(socket_fd, pi->packet, pi->packet_len, 0,
					   (sockaddr *) &receiver_addr, sizeof(sockaddr));                  /* TO-DO handle error */
			}

			last_check_time = current_time;
		}

		// if there are more data and available window
		if (!handler.is_all_sent() && handler.has_idle_window()) {

			pi = handler.generate_next_packet();

			// set send_time
			gettimeofday(&tv, nullptr);
			current_time = tv.tv_sec * 1000000 + tv.tv_usec;
			pi->send_time = current_time;

			if (pi->seq == 0) printf("[send data] NORMAL / (%d)\n", pi->packet_len);
			else printf("[send data] NORMAL %d (%d)\n", (pi->seq - 1) * DATA_PACKET_DATA_LENGTH, pi->packet_len);

			sendto(socket_fd, pi->packet, pi->packet_len, 0,
				   (sockaddr *) &receiver_addr, sizeof(sockaddr));                      /* TO-DO handle error */
		}

		// select, receive ack
		FD_ZERO(&read_set);
		FD_SET(socket_fd, &read_set);
		select(socket_fd + 1, &read_set, nullptr, nullptr, &select_timeout_tv);

		// if a ack arrives
		if (FD_ISSET(socket_fd, &read_set)) {
			recv_len = recvfrom(socket_fd, buff, BUFF_LENGTH, 0, nullptr, nullptr);     /* TO-DO handle error */

			// if still receiving ack after all sent, reset resend count
			if (handler.is_all_sent()) all_sent_retry_count = 0;

			if (validate_packet((uint16_t) recv_len)) {
				// extract ack
				ack = ntohs(*(uint16_t *) (buff + ACK_PACKET_HEADER_ACK_POSITION));

				// check finish
				if (ack == ACK_FINISH_FLAG) {
					printf("[completed] finish\n");

					break;
				}

				// forward to handler if there's a valid ack
				if (ack != ACK_NO_VALUE_FLAG) handler.recv_ack(ack);

				// extract accumulative ack
				acc_ack = ntohs(*(uint16_t *) (buff + ACK_PACKET_HEADER_ACC_ACK_POSITION));
				handler.update_send_base(acc_ack);
			} else {
				// if the packet corrupts, drop it
				printf("[recv corrupt packet]\n");
			}
		}
	}
}

// validate the received packet
bool Runner::validate_packet(uint16_t len) {
	if (len != ACK_PACKET_LENGTH) return false;

	uint16_t fcs = 0xffff;

	char curr;

	// for header
	for (int i = 0; i < ACK_PACKET_HEADER_CHECKSUM_POSITION; i++) {
		curr = buff[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	// for data
	for (int i = ACK_PACKET_HEADER_LENGTH; i < len; i++) {
		curr = buff[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	// for checksum
	for (int i = ACK_PACKET_HEADER_CHECKSUM_POSITION; i < ACK_PACKET_HEADER_CHECKSUM_POSITION + 2; i++) {
		curr = buff[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	// magic number for CRC-ITU
	return fcs == 0xf0b8;
}

// release all allocated memory
Runner::~Runner() {
	delete buff;
}
