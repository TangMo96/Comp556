#ifndef ROUTINGPROTOCOLIMPL_H
#define ROUTINGPROTOCOLIMPL_H


#include <list>
#include <map>

#include "Node.h"
#include "RoutingProtocol.h"


#define SPECIAL_ID 0xffff
// the initial ID of a neighbor


// every entry in the routing table (distance vector)
struct rt_entry {
	uint32_t cost;               // the cost to the dest, in ms
	uint16_t out_port;           // the port to the dest
	uint32_t last_update;        // the last update time of the entry, in ms

	rt_entry(uint32_t c, uint16_t o, uint32_t l) {
		cost = c;
		out_port = o;
		last_update = l;
	}
};

// every entry in the ports list (the status of every port)
struct pl_entry {
	uint16_t neighbor_id;        // the id of the neighbor
	uint32_t cost;               // the cost to the dest, in ms
	uint32_t last_ping;          // the last time that a ping was sent, in ms
	uint32_t last_update;        // the last time that a pong was received, in ms

	pl_entry(uint16_t n, uint32_t c, uint32_t lp, uint32_t lu) {
		neighbor_id = n;
		cost = c;
		last_ping = lp;
		last_update = lu;

	}
};

enum alarm_type {
	ALARM_CHECK_PL,
	ALARM_CHECK_RT
};


class RoutingProtocolImpl : public RoutingProtocol {

public:
	explicit RoutingProtocolImpl(Node *n);
	~RoutingProtocolImpl();

	void init(uint16_t num_ports, uint16_t router_id, eProtocolType protocol_type);
	// As discussed in the assignment document, your RoutingProtocolImpl is
	// first initialized with the total number of ports on the router,
	// the router's ID, and the protocol type (P_DV or P_LS) that
	// should be used. See global.h for definitions of constants P_DV
	// and P_LS.

	void handle_alarm(void *data);
	// As discussed in the assignment document, when an alarm scheduled by your
	// RoutingProtoclImpl fires, your RoutingProtocolImpl's
	// handle_alarm() function will be called, with the original piece
	// of "data" memory supplied to set_alarm() provided. After you
	// handle an alarm, the memory pointed to by "data" is under your
	// ownership and you should free it if appropriate.

	void recv(uint16_t port, void *packet, uint16_t size);
	// When a packet is received, your recv() function will be called
	// with the port number on which the packet arrives from, the
	// pointer to the packet memory, and the size of the packet in
	// bytes. When you receive a packet, the packet memory is under
	// your ownership and you should free it if appropriate. When a
	// DATA packet is created at a router by the simulator, your
	// recv() function will be called for such DATA packet, but with a
	// special port number of SPECIAL_PORT (see global.h) to indicate
	// that the packet is generated locally and not received from
	// a neighbor router.


	void check_pl();
	// check every port and send ping / set to dead if necessary

	void check_rt();
	// check every routing table entry and update if necessary

	void recv_ping(uint16_t port, char *packet);
	// recv ping packet from a port

	void recv_pong(uint16_t port, char *packet);
	// recv pong packet from a port

	void recv_dv(uint16_t port, char *packet, uint16_t size);
	// recv DV from a neighbor

	void recv_ls(uint16_t port, char *packet, uint16_t size);
	// recv LS from a neighbor

	void recv_data(uint16_t port, char *packet, uint16_t size);
	// recv data

	void send_ping(uint16_t port);
	// send ping to a neighbor

	void send_dv();
	// send DV to every neighbor

	bool update_dv_rt_by_port(uint16_t port, uint32_t prev_cost, uint32_t new_cost);
	// update the routing table triggered by the RTT change of a neighbor

	bool update_dv_rt_by_dv(uint16_t port, map<uint16_t, uint16_t> dv);
	// update the routing table by the DV received from a neighbor

	void update_dv_rt_entry_by_pl(uint16_t id, rt_entry *entry);
	// update a rt_entry if the cost of direct routing (if it's a neighbor) is smaller than its current cost


private:
	Node *sys;                                   // To store Node object; used to access GSR9999 interfaces

	uint16_t ports_num;                          // the number of ports
	uint16_t router_id;                          // the id of current router
	eProtocolType protocol_type;                 // the protocol that is currently active

	uint32_t dv_last_sent;                       // the last time that DVs were sent

	unordered_map<uint16_t, pl_entry *> pl;      // the ports list (the key is the index: 0, 1, 2, ...)
	unordered_map<uint16_t, rt_entry *> rt;      // the routing table (the key is the dest router id)
};


#endif
