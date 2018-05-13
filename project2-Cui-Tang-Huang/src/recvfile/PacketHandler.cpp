#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <sys/stat.h>

#include "PacketHandler.h"


PacketHandler::PacketHandler() {
	init();
}

// init the handler
void PacketHandler::init() {
	// init window
	window = new packet_info[WINDOW_COUNT];
	for (int i = 0; i < WINDOW_COUNT; i++) {
		window[i].has_packet = false;
		window[i].data = new char[DATA_PACKET_DATA_LENGTH + 2];
	}

	// init values
	next_ack = 0;
	last_packet_seq = 0;

	finished = false;

	file_path = nullptr;
}

// receive a packet and return the ack
uint16_t PacketHandler::recv(char *packet, uint16_t len) {
	uint16_t seq = ntohs(*(uint16_t *) (packet + DATA_PACKET_HEADER_SEQ_POSITION));

	if (seq < next_ack || seq >= (next_ack + WINDOW_COUNT)) {
		// if out of window, return accumulative ack only
		if (seq == 0) printf("[recv data] / (%d) IGNORED (out-of-window)\n", len);
		else printf("[recv data] %d (%d) IGNORED (out-of-window)\n", (seq - 1) * DATA_PACKET_DATA_LENGTH, len);

		return ACK_NO_VALUE_FLAG;
	} else {
		// extract and cache data
		packet_info *pi = &window[seq % WINDOW_COUNT];

		// if duplicate, return accumulative ack only
		if (pi->has_packet)	{
			if (seq == 0) printf("[recv data] / (%d) IGNORED (duplicate)\n", len);
			else printf("[recv data] %d (%d) IGNORED (duplicate)\n", (seq - 1) * DATA_PACKET_DATA_LENGTH, len);

			return ACK_NO_VALUE_FLAG;
		}

		pi->has_packet = true;
		pi->seq = seq;

		memcpy(pi->data, packet + DATA_PACKET_HEADER_LENGTH, (size_t) (len - DATA_PACKET_HEADER_LENGTH));

		// if the expecting ack arrives, write to file
		if (seq == next_ack) {
			if (seq == 0) printf("[recv data] / (%d) ACCEPTED (in-order)\n", len);
			else printf("[recv data] %d (%d) ACCEPTED (in-order)\n", (seq - 1) * DATA_PACKET_DATA_LENGTH, len);

			write_file();
		} else {
			if (seq == 0) printf("[recv data] / (%d) ACCEPTED (out-of-order)\n", len);
			else printf("[recv data] %d (%d) ACCEPTED (out-of-order)\n", (seq - 1) * DATA_PACKET_DATA_LENGTH, len);
		}

		return seq;
	}
}

// write to file (start from next_ack)
void PacketHandler::write_file() {
	// handle specially for first packet
	if (!file_path && next_ack == 0) {
		init_file(window[0].data);

		window[0].has_packet = false;

		next_ack++;
	}

	// keep writing until a no-data window
	auto curr_index = (uint16_t) (next_ack % WINDOW_COUNT);
	packet_info *curr_pi = &window[curr_index];

	while (curr_pi->has_packet) {
		if (curr_pi->seq == last_packet_seq) {
			// if is the last packet, extract actual length
			uint16_t len = ntohs(*(uint16_t *) (curr_pi->data));

			fout.write(curr_pi->data + 2, len);

			finished = true;
		} else {
			fout.write(curr_pi->data, DATA_PACKET_DATA_LENGTH);
		}

		// trigger the flag
		curr_pi->has_packet = false;

		// move forward
		next_ack++;
		if (++curr_index == WINDOW_COUNT) curr_index = 0;

		if (finished) break;

		curr_pi = &window[curr_index];
	}
}

// open file according to the file path
void PacketHandler::init_file(char *file_info) {
	// extract file length (in packets)
	last_packet_seq = ntohs(*(uint16_t *) (file_info));

	// extract file path
	size_t path_len = ntohs(*(uint16_t *) (file_info + 2));
	file_path = new char[path_len + 5];
	memcpy(file_path, file_info + 4, path_len);

	// check subdir
	char *subdir = strrchr(file_path, '/');
	if (subdir) {
		char origin = *subdir;

		*subdir = '\0';
		mkdir(file_path, 0755);
		*subdir = origin;
	}

	// the postfix
	memcpy(file_path + path_len, ".recv", 5);

	// open file
	fout.open(file_path, std::ios::binary | std::ios::app | std::ios::out);
}

// whether all packets are received successfully
bool PacketHandler::is_finished() {
	return finished;
}

// return the next_ack
uint16_t PacketHandler::get_next_ack() {
	return next_ack;
}

// release all allocated memory
PacketHandler::~PacketHandler() {
	fout.close();

	for (int i = 0; i < WINDOW_COUNT; i++)
		delete window[i].data;
	delete file_path;
	delete []window;
}
