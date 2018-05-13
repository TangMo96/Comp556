#include <iostream>
#include <cstring>
#include <arpa/inet.h>
#include <fcntl.h>

#include "helper.h"
#include "Runner.h"


Runner::Runner(uint16_t port) {
	init(port);

	config_socket();
}

// init all values
void Runner::init(uint16_t port) {
	// init local address info
	memset(&local_addr, 0, sizeof(local_addr));
	local_addr.sin_family = AF_INET;
	local_addr.sin_addr.s_addr = INADDR_ANY;
	local_addr.sin_port = htons(port);

	// init buff
	buff = new char[BUFF_LENGTH];
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

// start listening
void Runner::start() {
	// for select
	fd_set read_set;
	timeval select_timeout_tv;
	select_timeout_tv.tv_usec = SELECT_TIMEOUT_TIME;
	select_timeout_tv.tv_sec = 0;

	// for socket
	socklen_t sockaddr_len = sizeof(sockaddr);
	uint16_t packet_len;
	uint16_t recv_ret;

	// begin to receive
	while (true) {
		// select
		FD_ZERO(&read_set);
		FD_SET(socket_fd, &read_set);
		select(socket_fd + 1, &read_set, nullptr, nullptr, &select_timeout_tv);             /* TO-DO handle error */

		// new packet arrives
		if (FD_ISSET(socket_fd, &read_set)) {
			packet_len = (uint16_t) recvfrom(socket_fd, buff, BUFF_LENGTH, 0,
											 (sockaddr *) &sender_addr, &sockaddr_len);     /* TO-DO handle error */

			// validate a packet
			if (validate_packet(packet_len)) {
				recv_ret = handler.recv(buff, packet_len);

				generate_ack_packet((uint16_t) recv_ret, false);

				// include padding (46 - 20 - 8) and send
				sendto(socket_fd, buff, ACK_PACKET_LENGTH, 0,
					   (sockaddr *) &sender_addr, sizeof(sockaddr));                    /* TO-DO handle error */

				// check finish
				if (handler.is_finished()) {
					generate_ack_packet(0, true);

					// include padding (46 - 20 - 8) and send n times
					for (int i = 0; i < FINISH_ACK_SEND_TIME; i++)
						sendto(socket_fd, buff, ACK_PACKET_LENGTH, 0,
						   (sockaddr *) &sender_addr, sizeof(sockaddr));               /* TO-DO handle error */

					break;
				}
			} else {
				// if the package corrupts, drop it and send accumulative ack only
				printf("[recv corrupt packet]\n");

				generate_ack_packet(ACK_NO_VALUE_FLAG, false);

				// include padding (46 - 20 - 8) and send
				sendto(socket_fd, buff, ACK_PACKET_LENGTH, 0,
					   (sockaddr *) &sender_addr, sizeof(sockaddr));                    /* TO-DO handle error */
			}
		}
	}
}

// generate ack packet with given ack number
void Runner::generate_ack_packet(uint16_t ack, bool is_finished) {
	// generate ack
	if (is_finished) *(uint16_t *) (buff + ACK_PACKET_HEADER_ACK_POSITION) = (uint16_t) htons(ACK_FINISH_FLAG);
	else *(uint16_t *) (buff + ACK_PACKET_HEADER_ACK_POSITION) = (uint16_t) htons(ack);

	// generate accumulative ack
	*(uint16_t *) (buff + ACK_PACKET_HEADER_ACC_ACK_POSITION) = (uint16_t) htons(handler.get_next_ack());

	generate_packet_checksum();
}

// validate a packet
bool Runner::validate_packet(uint16_t len) {
	if (len < DATA_PACKET_HEADER_LENGTH) return false;

	uint16_t fcs = 0xffff;

	char curr;

	// for header
	for (int i = 0; i < DATA_PACKET_HEADER_CHECKSUM_POSITION; i++) {
		curr = buff[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	// for data
	for (int i = DATA_PACKET_HEADER_LENGTH; i < len; i++) {
		curr = buff[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	// for checksum
	for (int i = DATA_PACKET_HEADER_CHECKSUM_POSITION; i < DATA_PACKET_HEADER_CHECKSUM_POSITION + 2; i++) {
		curr = buff[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	// magic number for CRC-ITU
	return fcs == 0xf0b8;
}

// generate checksum
void Runner::generate_packet_checksum() {
	uint16_t fcs = 0xffff;

	char curr;

	// for header
	for (int i = 0; i < ACK_PACKET_HEADER_CHECKSUM_POSITION; i++) {
		curr = buff[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	// for data (include padding to 18)
	for (int i = ACK_PACKET_HEADER_LENGTH; i < ACK_PACKET_LENGTH; i++) {
		curr = buff[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	*(uint16_t *) (buff + ACK_PACKET_HEADER_CHECKSUM_POSITION) = ~fcs;
}

// release all allocated memory
Runner::~Runner() {
	delete buff;
}
