#ifndef SENDFILE_RUNNER_H
#define SENDFILE_RUNNER_H


#include <netinet/in.h>

#include "PacketHandler.h"


class Runner {

private:
	PacketHandler handler;                   // the handler to handle packets

	int socket_fd;                           // the fd of the socket

	sockaddr_in local_addr;                  // the address info of localhost
	sockaddr_in receiver_addr;               // the address info of the sender

	char *buff;                              // the receiving buff

	void init(char *, uint16_t, char *);
	void config_socket();
	bool validate_packet(uint16_t);


public:
	Runner(char *, uint16_t, char *);
	void start();
	~Runner();

};


#endif
