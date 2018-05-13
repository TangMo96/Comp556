#ifndef EVENT_H
#define EVENT_H

#include "global.h"
#include "RoutingProtocol.h"
#include "Link.h"
#include "Node.h"

/* All the possible events. The dispatch method of each event is called when it is taken from the queue*/
class Event {
  
 public:
  Event(){};
  Event(unsigned int time) : time(time) {}
  virtual ~Event() {};
  unsigned int time;
  virtual void dispatch(){} // function called when event is taken from the queue
  virtual void pt() {}      // print out the type of event 
};


class Event_Change_Delay : public Event {
	private:
		Link *link;
		unsigned int delay;
	public:
		Event_Change_Delay(Link *ln, unsigned int newdelay, unsigned int time) : Event(time), link(ln), delay(newdelay) {}
		void dispatch();
		void pt() { cout << "Event_Change_Delay (" << link->get_node1()->id << "," << link->get_node2()->id << ")" << endl; }
};

class Event_Link_Die : public Event {
 public:
  Event_Link_Die(Link * ln, unsigned int time) : Event(time), dying_link(ln) {}
  void dispatch();
  void pt(){ cout<<"Event_Link_Die ("<<dying_link->get_node1()->id<<","<<dying_link->get_node2()->id<<")\n"; }
 private:
  Link * dying_link;
};

class Event_Link_Come_Up : public Event {
 public:
  Event_Link_Come_Up(Link * ln, unsigned int time) : Event(time), upcoming_link(ln) {}
  void dispatch();
  void pt() {cout<<"Event_Link_Come_Up ("<<upcoming_link->get_node1()->id<<","<<upcoming_link->get_node2()->id<<")\n";}
 private:
  Link * upcoming_link;
};

class Event_Xmit_Pkt_End_To_End_Generic : public Event {
 public:
  Event_Xmit_Pkt_End_To_End_Generic(Node * src, Node * dst, void * pkt, unsigned short size, unsigned int time) : Event(time), src(src), dst(dst), pkt(pkt), size(size) {}
  void dispatch();
  void pt() {
    cout<<"Event_Xmit_Data_Pkt source node ";
    cout<<src->id<<" destination node "<<dst->id<<" packet type is DATA\n";
  }
 private:
  Node * src;
  Node * dst;
  void * pkt;
  unsigned short size;
};

class Event_Xmit_Pkt_On_Link_Generic : public Event {
 public:
  Event_Xmit_Pkt_On_Link_Generic(Node * dest, unsigned short dport, Link * ln, void * pkt, unsigned short size, unsigned int time) : Event(time), dest(dest), dport(dport), ln(ln), pkt(pkt), size(size) {}
  void dispatch();
  void pt();
 private:
  Node * dest;
  unsigned short dport;
  Link * ln;
  void * pkt;
  unsigned short size;
};

class Event_Recv_Pkt_Generic : public Event {
 public:
  Event_Recv_Pkt_Generic(Node * dest, unsigned short dport, void * pkt, unsigned short size, unsigned int time) : Event(time), dest(dest), dport(dport), pkt(pkt), size(size) {}
  void dispatch();
  void pt();
  Node * dest;
  unsigned short dport;
  void * pkt;
  unsigned short size;
};

class Event_Alarm : public Event {
 public:
  Event_Alarm(RoutingProtocol *r, void *d, unsigned int time, int id) : Event(time), rp(r), data(d), router_id(id) {}
  void dispatch();
  void pt() {cout<<"Event_Alarm on node "<< router_id <<"\n";}

  RoutingProtocol *rp;
  void *data;

 private:
  int router_id;
};


#endif 
