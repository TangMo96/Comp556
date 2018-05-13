#ifndef RECVFILE_RUNNER_H
#define RECVFILE_RUNNER_H


#include <netinet/in.h>

#include "PacketHandler.h"


class Runner {

private:
	PacketHandler handler;          // the handler to handle data

	int socket_fd;                  // the fd of the socket

	sockaddr_in local_addr;         // the address info of localhost
	sockaddr_in sender_addr;        // the address info of the sender

	char *buff;                     // the receiving & sending buff

	void init(uint16_t);
	void config_socket();
	bool validate_packet(uint16_t);
	void generate_ack_packet(uint16_t, bool);
	void generate_packet_checksum();


public:
	explicit Runner(uint16_t);
	void start();
	~Runner();

};


#endif
