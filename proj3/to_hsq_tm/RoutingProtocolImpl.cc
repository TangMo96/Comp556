// #include <w32api/winsock2.h>
#include <arpa/inet.h>

#include "RoutingProtocolImpl.h"


RoutingProtocolImpl::RoutingProtocolImpl(Node *n) : RoutingProtocol(n) {
	sys = n;
}

RoutingProtocolImpl::~RoutingProtocolImpl() {
	for (auto it: rt)
		delete it.second;

	for (auto it: pl)
		delete it.second;
}

void RoutingProtocolImpl::init(uint16_t ports_num, uint16_t router_id, eProtocolType protocol_type) {
	this->ports_num = ports_num;
	this->router_id = router_id;
	this->protocol_type = protocol_type;

	// init ports
	for (uint16_t i = 0; i < ports_num; i++)
		pl.insert(make_pair(i, new pl_entry(SPECIAL_ID, INFINITY_COST, 1, 0)));

	dv_last_sent = 0;

	// begin checking
	check_pl();
	check_rt();
}

void RoutingProtocolImpl::handle_alarm(void *data) {
	switch(((char *) data)[0]) {
		case ALARM_CHECK_PL:
			check_pl();
			break;

		case ALARM_CHECK_RT:
			check_rt();
			break;

		default:
			break;
	}

	delete (char *) data;
}

void RoutingProtocolImpl::recv(uint16_t port, void *packet, uint16_t size) {
	auto pac = (char *) packet;

	switch(pac[0]) {
		case PING:
			recv_ping(port, pac);
			break;

		case PONG:
			recv_pong(port, pac);
			break;

		case DV:
			recv_dv(port, pac, size);
			break;

		case LS:
			recv_ls(port, pac, size);
			break;

		case DATA:
			recv_data(port, pac, size);
			break;

		default:
			break;
	}
}

void RoutingProtocolImpl::check_pl() {
	auto curr = sys->time();

	for (auto it: pl) {
		auto pe = it.second;

		// check ping
		if (pe->last_ping == 1 || curr - pe->last_ping == 10000) {
			send_ping(it.first);

			pe->last_ping = curr;
		}

		// check active
		if (curr - pe->last_update > 15000 && pe->cost != INFINITY_COST) {
			pe->cost = INFINITY_COST;

			if (protocol_type == P_DV) {
				if (update_dv_rt_by_port(it.first, 0, INFINITY_COST)) send_dv();
			} else {
				// TODO update LS routing table
			}
		}
	}

	auto alarm = new char[1];
	alarm[0] = ALARM_CHECK_PL;
	sys->set_alarm(this, 1000, alarm);
}

void RoutingProtocolImpl::check_rt() {
	auto curr = sys->time();

	bool updated = false;

	// check active
	for (auto it: rt) {
		auto re = it.second;

		// dead after 45s
		if (curr -re->last_update > 45000) {
			if (protocol_type == P_DV) {
				auto prev_cost = re->cost;

				re->cost = INFINITY_COST;
				re->last_update = curr;

				update_dv_rt_entry_by_pl(it.first, re);

				if (re->cost != prev_cost) updated = true;
			} else {
				// TODO update LS routing table
			}
		}
	}

	// send dv every 30s
	if (curr - dv_last_sent == 30000 || updated) {
		send_dv();

		if (curr - dv_last_sent == 30000) dv_last_sent = curr;
	}

	auto alarm = new char[1];
	alarm[0] = ALARM_CHECK_RT;
	sys->set_alarm(this, 1000, alarm);
}


void RoutingProtocolImpl::recv_ping(uint16_t port, char *packet) {
	auto sender_id = ntohs(*(uint16_t *) (packet + 4));

	// generate packet
	packet[0] = (char) PONG;
	*(uint16_t *) (packet + 4) = htons(router_id);
	*(uint16_t *) (packet + 6) = htons(sender_id);

	sys->send(port, packet, 12);
}

void RoutingProtocolImpl::recv_pong(uint16_t port, char *packet) {
	auto curr = sys->time();

	// retrieve data
	auto sender_id = ntohs(*(uint16_t *) (packet + 4));
	auto timestamp = ntohl(*(uint32_t *) (packet + 8));

	pl_entry *pe = pl.at(port);

	auto prev_cost = pe->cost;
	auto new_cost = curr - timestamp;

	// update info
	pe->neighbor_id = sender_id;
	pe->cost = new_cost;
	pe->last_update = curr;

	if (protocol_type == P_DV) {
		if (update_dv_rt_by_port(port, prev_cost, new_cost)) send_dv();
	} else {
		// TODO update LS routing table
	}

	delete packet;

//	 *************************
//	cout << endl << "#" << router_id << " after recv_pong() from #" << sender_id << endl;
//	for (auto it: rt) {
//		cout << "dest: " << it.first << ", cost: " << it.second->cost << ", port: " << it.second->out_port << endl;
//	}
//	cout << endl;
//	 *************************
}

void RoutingProtocolImpl::recv_dv(uint16_t port, char *packet, uint16_t size) {
	if (pl.at(port)->cost == INFINITY_COST) return;

	map<uint16_t, uint16_t> map;

	// extract dvs
	for (int i = 0; i < (size - 8) / 4; i++) {
		auto id = ntohs(*(uint16_t *) (packet + 8 + i * 4));
		auto cost = ntohs(*(uint16_t *) (packet + 8 + i * 4 + 2));

		// route to self is useless
		if (id == router_id) continue;

		map.insert(make_pair(id, cost));
	}

	if (update_dv_rt_by_dv(port, map)) send_dv();

//	 *************************
//	cout << endl << "#" << router_id << " after recv_dv() from #" << pl.at(port)->neighbor_id << endl;
//	for (auto it: rt) {
//		cout << "dest: " << it.first << ", cost: " << it.second->cost << ", port: " << it.second->out_port << endl;
//	}
//	cout << endl;
//	 *************************
}

void RoutingProtocolImpl::recv_ls(uint16_t port, char *packet, uint16_t size) {
	// TODO recv LS packet
}

void RoutingProtocolImpl::recv_data(uint16_t port, char *packet, uint16_t size) {
	auto dest_id = ntohs(*(uint16_t *) (packet + 6));

	// reach dest, delete
	if (dest_id == router_id) return delete packet;

	// set source id
	if (port == SPECIAL_PORT) *(uint16_t *) (packet + 4) = htons(router_id);

	// search routing table
	if (rt.find(dest_id) != rt.end()) {
		auto re = rt.at(dest_id);

		if (re->cost != INFINITY_COST) return sys->send(re->out_port, packet, size);
	}

	delete packet;
}


void RoutingProtocolImpl::send_ping(uint16_t port) {
	auto packet = new char[12];

	// generate packet;
	packet[0] = (char) PING;
	*(uint16_t *) (packet + 2) = htons(12);
	*(uint16_t *) (packet + 4) = htons(router_id);
	*(uint32_t *) (packet + 8) = htonl(sys->time());

	sys->send(port, packet, 12);
}

void RoutingProtocolImpl::send_dv() {
	if (rt.empty()) return;

	list<uint16_t > ids;

	// find all ports that are accessible
	for (auto it: rt)
		if (it.second->cost != INFINITY_COST) ids.push_back(it.first);

	auto size = 8 + ids.size() * 4;

	// send DV to all neighbors
	for (uint16_t i = 0; i < ports_num; i++) {
		auto pe = pl.at(i);

		// check neighbor existence
		if (pe->neighbor_id == SPECIAL_ID || pe->cost == INFINITY_COST) continue;

		auto packet = new char[size];

		// generate header
		packet[0] = (char) DV;
		*(uint16_t *) (packet + 2) = htons((uint16_t) size);
		*(uint16_t *) (packet + 4) = htons(router_id);
		*(uint16_t *) (packet + 6) = htons(pe->neighbor_id);

		// generate body
		uint16_t count = 0;
		for (auto id: ids) {
			auto re = rt.at(id);

			*(uint16_t *) (packet + 8 + count * 4) = htons(id);

			// poison reverse
			if (re->out_port == i)
				*(uint16_t *) (packet + 8 + count * 4 + 2) = htons((uint16_t) INFINITY_COST);
			else
				*(uint16_t *) (packet + 8 + count * 4 + 2) = htons((uint16_t) re->cost);

			count++;
		}

		sys->send(i, packet, (uint16_t) size);
	}
}

bool RoutingProtocolImpl::update_dv_rt_by_port(uint16_t port, uint32_t prev_cost, uint32_t new_cost) {
	auto curr = sys->time();

	bool updated = false;

	if (prev_cost != INFINITY_COST && prev_cost != new_cost) {
		for (auto it: rt) {
			auto re = it.second;

			// update entry that go through this port
			if (re->out_port == port && re->cost != INFINITY_COST) {
				auto old_cost = re->cost;

				if (new_cost == INFINITY_COST)
					re->cost = INFINITY_COST;
				else
					re->cost += new_cost - prev_cost;

				re->last_update = curr;

				updated = true;

				// check direct routing
				if (re->cost > old_cost) update_dv_rt_entry_by_pl(it.first, re);
			}
		}
	}

	if (new_cost != INFINITY_COST) {
		auto pe = pl.at(port);

		if (rt.find(pe->neighbor_id) != rt.end()) {
			auto re = rt.at(pe->neighbor_id);

			// check self direct routing
			if (new_cost < re->cost) {
				re->cost = new_cost;
				re->out_port = port;
				re->last_update = curr;

				updated = true;
			}
		} else {
			// add new entry
			rt.insert(make_pair(pe->neighbor_id, new rt_entry(pe->cost, port, curr)));

			updated = true;
		}
	}

	return updated;
}

bool RoutingProtocolImpl::update_dv_rt_by_dv(uint16_t port, map<uint16_t, uint16_t> dv) {
	auto curr = sys->time();

	auto pe = pl.at(port);

	bool updated = false;

	for (auto it: rt) {
		auto re = it.second;

		// route to this neighbor should not be decided by himself
		if (it.first == pe->neighbor_id) continue;

		if (dv.find(it.first) != dv.end()) {
			auto sub_cost = dv.at(it.first);

			// update rt entry that go through this port
			if (re->out_port == port) {
				auto prev_cost = re->cost;

				if (sub_cost == INFINITY_COST)
					re->cost = INFINITY_COST;
				else
					re->cost = pe->cost + sub_cost;

				re->last_update = curr;

				if (re->cost != prev_cost) {
					updated = true;

					// check direct routing
					if (re->cost > prev_cost) update_dv_rt_entry_by_pl(it.first, re);
				}
			} else if (pe->cost + sub_cost < re->cost) {
				// a faster route is selected
				re->cost = pe->cost + sub_cost;
				re->out_port = port;
				re->last_update = curr;

				updated = true;
			}
		} else if (re->out_port == port && re->cost != INFINITY_COST) {
			// this port can no longer reach the dest
			re->cost = INFINITY_COST;
			re->last_update = curr;

			updated = true;

			// check direct routing
			update_dv_rt_entry_by_pl(it.first, re);
		}
	}

	// add new entries
	for (auto it: dv) {
		if (rt.find(it.first) == rt.end()) {
			rt.insert(make_pair(it.first, new rt_entry(pe->cost + it.second, port, curr)));

			updated = true;
		}
	}

	return updated;
}

void RoutingProtocolImpl::update_dv_rt_entry_by_pl(uint16_t id, rt_entry *entry) {
	for (auto it: pl) {
		auto pe = it.second;

		if (id == pe->neighbor_id && pe->cost < entry->cost) {
			entry->cost = pe->cost;
			entry->out_port = it.first;

			return;
		}
	}
}
