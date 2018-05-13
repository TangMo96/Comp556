#include "Node.h"
#include "Simulator.h"
#include <string.h>

extern Simulator * sim;

ostream& Node::operator<<(ostream & os) {
  os<<"Node("<<id<<") ";
  return os;
}

void Node::add_link(Link * link) {
  link_vector.push_back(link);
  cout<<link<<" has been added\n";
  return;
}

unsigned int Node::time() {
  return sim->time();
}

void Node::set_alarm(RoutingProtocol *r, unsigned int duration, void *d) {
  sim->set_alarm(r, duration, d);
}

void Node::send(unsigned short port, void *packet, unsigned short size) {
  // Need to map port into a link and send
  
  Node *rcpt_node = (*(link_vector[port]->get_node1()) == *this) ? link_vector[port]->get_node2() : link_vector[port]->get_node1();

  unsigned short rcpt_port = rcpt_node->get_link_port(link_vector[port]);
  link_vector[port]->send_generic(packet, size, rcpt_port, rcpt_node);
}

void Node::recv_packet(unsigned short port, void * pkt, unsigned short size) {
  void *p = malloc(size);
  memcpy(p, pkt, size);
  free(pkt);
  rp->recv(port, p, size);
}

unsigned short Node::get_link_port(Link *l) {
  unsigned int i;

  for (i = 0; i < link_vector.size(); i++) {
    Link * curr_link = link_vector[i];
    if (curr_link == l) {
      return i;
    }
  }

  cout<<"Unable to find link port number, this is bad. Exiting.\n";
  exit(-1);
}

