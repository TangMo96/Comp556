#ifndef NODE_H
#define NODE_H

#include "global.h"
#include "Link.h"

class Link;
class RoutingProtocol;

class Node {

 public:
  Node(unsigned short a) : id(a) {};
  ~Node() {};
  ostream& operator<<(ostream &);
  vector<Link*> link_vector;
  bool operator==(Node & nd) { return nd.id == id;}
  void add_link(Link *link);
  void recv_packet(unsigned short port, void * pkt, unsigned short size);
  unsigned short get_link_port(Link *l);
  unsigned short id;
  RoutingProtocol *rp;


  /*******************************************/
  /* BEGIN Interfaces needed by student code */
  /*******************************************/

  unsigned int time();
  // returns system time in milliseconds since system bootup

  void set_alarm(RoutingProtocol *r, unsigned int duration, void *data);
  // set an alarm to be fired in "duration" milliseconds. caller must
  // be a RoutingProtocol object (RoutingProtoclImpl is fine since
  // it's a subclass) and provide a pointer to itself in parameter
  // "r". "r" is used to get back to the caller when the alarm
  // fires. "duration" is measured in milliseconds. "data" may point to
  // an arbitrary piece of memory provided by caller, which will be
  // handed back to the caller when the alarm fires (see
  // handle_alarm() in RoutingProtocolImpl.h). Do not free or modify
  // memory pointed to by "data" after calling set_alarm(). set_alarm()
  // assumes ownership of the memory until the alarm fires and
  // handle_alarm() is called.

  void send(unsigned short port, void *packet, unsigned short size);
  // send a packet in memory pointed to by "packet" of size "size"
  // bytes out on port number "port". do not free or modify the packet
  // memory after passing it to the send() function. the send function
  // assumes ownership of the packet memory.

  /*****************************************/
  /* END Interfaces needed by student code */
  /*****************************************/

};
#endif
