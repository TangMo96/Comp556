#include <iostream>
#include <cstring>
#include <netinet/in.h>
#include <cmath>

#include "PacketHandler.h"


// init the handler
void PacketHandler::init(char *fp) {
	// init window
	window = new packet_info[WINDOW_COUNT];
	for (int i = 0; i < WINDOW_COUNT; i++) {
		window[i].is_acked = false;
		window[i].packet = new char[DATA_PACKET_HEADER_LENGTH + DATA_PACKET_DATA_LENGTH + 2];
	}

	// init values
	seq_base = 0;
	seq_next = 0;

	all_sent = false;
	first = true;

	// init file
	size_t file_path_len = strlen(fp);
	file_path = new char[file_path_len];
	memcpy(file_path, fp, file_path_len);

	// open file
	fin.open(file_path, std::ios::binary | std::ios::in);                           /* TO-DO handle error */

	// calculate file length
	fin.seekg(0, std::ios::end);
	file_length = fin.tellg();

	// back to the file beginning
	fin.seekg(0, std::ios::beg);
	curr_file_pos = 0;
}

// return the [next packet to send]
packet_info *PacketHandler::generate_next_packet() {
	packet_info *pi = &window[seq_next % WINDOW_COUNT];

	// if it is the first packet, handle specially (send the file path and length)
	if (first) {
		// generate file length (in packets)
		auto total_packet_count = (uint16_t) (ceil((double) file_length / DATA_PACKET_DATA_LENGTH));
		*(uint16_t *) (pi->packet + DATA_PACKET_HEADER_LENGTH) = (uint16_t) htons(total_packet_count);

		// generate file path length (in bytes)
		auto file_path_len = (uint16_t) strlen(file_path);
		*(uint16_t *) (pi->packet + DATA_PACKET_HEADER_LENGTH + 2) = (uint16_t) htons(file_path_len);

		// generate file path
		memcpy(pi->packet + DATA_PACKET_HEADER_LENGTH +  4, file_path, file_path_len);

		// add padding if the packet length is less than (46 - 20 - 8 - *)
		uint16_t min_len = 18 - DATA_PACKET_HEADER_LENGTH - 4;
		if (file_path_len < min_len) file_path_len = min_len;

		pi->packet_len = (uint16_t) (DATA_PACKET_HEADER_LENGTH + 4 + file_path_len);

		first = false;
	} else {
		long pending_length = file_length - curr_file_pos;

		// if it is the last packet
		if (pending_length <= DATA_PACKET_DATA_LENGTH) {
			// add length info
			*(uint16_t *) (pi->packet + DATA_PACKET_HEADER_LENGTH) = (uint16_t) htons((uint16_t) pending_length);
			fin.read(pi->packet + DATA_PACKET_HEADER_LENGTH + 2, pending_length);

			// add padding if the packet length is less than (46 - 20 - 8 - *)
			uint16_t min_len = 18 - DATA_PACKET_HEADER_LENGTH - 2;
			if (pending_length < min_len) pending_length = min_len;

			pi->packet_len = (uint16_t) (DATA_PACKET_HEADER_LENGTH + 2 + pending_length);

			all_sent = true;
		} else {
			fin.read(pi->packet + DATA_PACKET_HEADER_LENGTH, DATA_PACKET_DATA_LENGTH);

			pi->packet_len = (uint16_t) (DATA_PACKET_HEADER_LENGTH + DATA_PACKET_DATA_LENGTH);
		}

		curr_file_pos += DATA_PACKET_DATA_LENGTH;
	}

	pi->is_acked = false;
	pi->seq = seq_next++;

	// generate seq
	*(uint16_t *) (pi->packet + DATA_PACKET_HEADER_SEQ_POSITION) = (uint16_t) htons(pi->seq);

	generate_checksum(pi);

	return pi;
}

// return all sent but not-acked and timeout packets
std::vector<packet_info *> *PacketHandler::get_not_acked_timeout_packets(time_t current_time) {
	vec.clear();

	packet_info *pi;

	int curr_index = seq_base % WINDOW_COUNT;
	int seq_base_copy = seq_base;

	while (seq_base_copy != seq_next) {
		pi = &window[curr_index];
		if (!pi->is_acked && (current_time - pi->send_time) > PACKET_TIMEOUT_TIME) vec.push_back(pi);

		if (vec.size() > MAX_RESEND_COUNT) break;

		if (++curr_index == WINDOW_COUNT) curr_index = 0;
		seq_base_copy++;
	}

	return &vec;
}

// recv an ack
void PacketHandler::recv_ack(uint16_t ack) {
	if (ack >= seq_base && ack < seq_next) {
		packet_info *pi = &window[ack % WINDOW_COUNT];

		// check duplication
		if (pi->is_acked) {
			if (ack == 0) printf("[recv ack] /\n");
			else printf("[recv ack] %d IGNORED (duplicate)\n", (ack - 1) * DATA_PACKET_DATA_LENGTH);
		} else {
			// accept
			if (ack == 0) printf("[recv ack] /\n");
			else printf("[recv ack] %d ACCEPTED\n", (ack - 1) * DATA_PACKET_DATA_LENGTH);

			pi->is_acked = true;
		}
	} else {
		// if the ack is not in the window, drop it
		if (ack == 0) printf("[recv ack] / IGNORED (out-of-window)\n");
		else printf("[recv ack] %d IGNORED (out-of-window)\n", (ack - 1) * DATA_PACKET_DATA_LENGTH);
	}
}

// update send base
void PacketHandler::update_send_base(uint16_t new_base) {
	if (new_base > seq_base && new_base <= seq_next) {
		int curr_index = seq_base % WINDOW_COUNT;
		int new_index = new_base % WINDOW_COUNT;
		while (curr_index != new_index) {
			window[curr_index].is_acked = true;

			if (++curr_index == WINDOW_COUNT) curr_index = 0;
		}

		seq_base = new_base;
	}
}

// whether has idle window space
bool PacketHandler::has_idle_window() {
	return (seq_next - seq_base) < WINDOW_COUNT;
}

// whether all data has been sent
bool PacketHandler::is_all_sent() {
	return all_sent;
}

// generate checksum
void PacketHandler::generate_checksum(packet_info * pi) {
	uint16_t fcs = 0xffff;

	char curr;

	// for header
	for (int i = 0; i < DATA_PACKET_HEADER_CHECKSUM_POSITION; i++) {
		curr = pi->packet[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	// for data
	for (int i = DATA_PACKET_HEADER_LENGTH; i < pi->packet_len; i++) {
		curr = pi->packet[i];
		fcs = (fcs >> 8) ^ CRC16_TABLE[(fcs ^ curr) & 0xff];
	}

	*(uint16_t *) (pi->packet + DATA_PACKET_HEADER_CHECKSUM_POSITION) = ~fcs;
}

PacketHandler::~PacketHandler() {
	fin.close();

	for (int i = 0; i < WINDOW_COUNT; i++)
		delete window[i].packet;

	delete []window;
	delete file_path;
}
