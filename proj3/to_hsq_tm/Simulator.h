#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "global.h"
#include "Node.h"
#include "Event.h"
#include "RoutingProtocol.h"

struct lteve {
  bool operator() (Event *e1, Event *e2) {
    return e1->time > e2->time;
  }
};

struct int_pair {
  int_pair(int first, int second) : first(first), second(second) {};
  int first;
  int second;
};

typedef struct int_pair pr;

namespace __gnu_cxx
{
  struct hash_pair {
    size_t operator()(const pr & one) const {
      return hash<int>()(one.first) + hash<int>()(one.second);
    }
  };
}

struct equal_pair {
  bool operator()(const pr one, const pr two) const {
    return ( ((one.first == two.first) && (one.second == two.second)) || ((one.first == two.second) && (one.second == two.first)));
  }
};

struct sim_pkt_header {
  unsigned char type;
  unsigned char reserved;
  unsigned short size;
  unsigned short src;
  unsigned short dst;
};

class Simulator {

 public:
  Simulator(char * filename) : filename(filename) {};
  ~Simulator() {cout<<"Simulator Destructor called\n";}
  void init(char *ptype); // initializes the simulator
  void run();  // the actual dequeueing is performed here
  void cleanup(); // any testing is performed here
  void add_node(Node * node);
  unsigned int time(); // returns global_time in unit of ms
  void set_alarm(RoutingProtocol *r, unsigned int duration, void *d);
  void init_routing_protocol(eProtocolType type); // calling student's code

  priority_queue < Event*, vector < Event* >, lteve > event_q;
  char * filename;
  unsigned int global_time; //global time
  eProtocolType protocol_type;
  unsigned int stop_time;

  //pyuan added
 private:
  string trim(string s);
  std::unordered_map<int, Node *> node_table;
  std::unordered_map<int_pair, Link *, hash_pair, equal_pair> link_table;
  //end of pyuan

  //twngan added
  std::unordered_map<const char *, int> protocol_table;
  //end of twngan

};


#endif
