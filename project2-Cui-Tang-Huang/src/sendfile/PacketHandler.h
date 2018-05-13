#ifndef SENDFILE_HANDLER_H
#define SENDFILE_HANDLER_H


#include <fstream>
#include <vector>

#include "helper.h"


class PacketHandler {

private:
	packet_info *window;                      // the sending window, caching sent packets

	uint16_t seq_base;                        // the seq number of the 'smallest' sent but not-acked packet
	uint16_t seq_next;                        // the seq number of the next will-be-sent packet

	bool all_sent;                            // whether all packets have been sent
	bool first;                               // whether no packet has been sent yet

	std::vector<packet_info *> vec;           // used in get_not_acked_packets()

	char *file_path;                          // the path of the file
	long file_length;                         // the length of the file (in bytes)
	long curr_file_pos;                       // curr position in the file (in bytes)
	std::ifstream fin;                        // file stream (to read file)

	void generate_checksum(packet_info *);


public:
	void init(char *);
	std::vector<packet_info *> *get_not_acked_timeout_packets(time_t);
	bool has_idle_window();
	bool is_all_sent();
	packet_info *generate_next_packet();
	void recv_ack(uint16_t);
	void update_send_base(uint16_t);
	~PacketHandler();

};


#endif
