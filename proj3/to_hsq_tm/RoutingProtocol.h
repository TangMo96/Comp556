#ifndef ROUTINGPROTOCOL_H
#define ROUTINGPROTOCOL_H

#include "global.h"

class Node;

class RoutingProtocol {
  protected:
    Node *sys;

  public:
    RoutingProtocol(Node *n) : sys(n) {}
    virtual ~RoutingProtocol() {};
    virtual void init(unsigned short num_ports, unsigned short router_id, eProtocolType protocol_type) = 0;
    virtual void handle_alarm(void *data) = 0;
    virtual void recv(unsigned short port, void *packet, unsigned short size) = 0;
};

#endif

