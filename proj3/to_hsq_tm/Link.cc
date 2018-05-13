#include "Link.h"
#include "Simulator.h"

extern Simulator * sim;

void Link::send_generic(void * pkt, unsigned short size, unsigned short dport, Node * dest) {
  Event * e = new Event_Xmit_Pkt_On_Link_Generic(dest, dport, this, pkt, size, sim->global_time);
  sim->event_q.push(e);
  return;
}

void Link::link_dies() {
  //  cout<<this<<" link has died\n";
  is_alive = false;
  return;
}

void Link::link_comes_up() {
  //  cout<<this<<" link has come alive. Yay (Undie)!!!\n";
  is_alive = true;
  return;
}

ostream& Link::operator<<(ostream& os)  {
  os<<"Link("<<id<<") ";
  return os;
}
