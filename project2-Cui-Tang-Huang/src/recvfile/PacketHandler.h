#ifndef RECVFILE_HANDLER_H
#define RECVFILE_HANDLER_H


#include <fstream>

#include "helper.h"


class PacketHandler {

private:
	packet_info *window;           // the receiving window, caching received file packets

	uint16_t next_ack;             // the expected sequence number of the next packet
	uint16_t last_packet_seq;      // the sequence number of the last packet

	bool finished;                 // whether the full file has been received

	char *file_path;               // the path where the received file should store
	std::ofstream fout;            // file stream

	void init();
	void init_file(char *);
	void write_file();


public:
	PacketHandler();
	uint16_t recv(char *, uint16_t);
	bool is_finished();
	uint16_t get_next_ack();
	~PacketHandler();

};


#endif
