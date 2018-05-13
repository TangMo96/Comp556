#ifndef LINK_H
#define LINK_H

#include "global.h"
#include "Node.h"

class Node;

class Link {

 public:
  Link(Node * node1, Node * node2, unsigned int delay, double loss_prob, int cost) : delay(delay), loss_prob(loss_prob), node1(node1), node2(node2), cost(cost) {is_alive = true; } 
  ~Link() {};
  unsigned short id;
  // All the rest our internal functions that are used by the simulator to act on links.
  void die();                 
  ostream& operator<<(ostream &);
  unsigned int get_delay() {return delay; }
  double get_loss_prob() {return loss_prob; }
  void link_dies();
  void link_comes_up();
  Node * get_node1() {return node1;}
  Node * get_node2() {return node2;}
  int get_cost() { return cost;}
  bool get_is_alive() {return is_alive;}
  void change_delay(unsigned int d) { delay = d; }

  void send_generic(void *pkt, unsigned short size, unsigned short dport, Node *); //send the packet to the recipient

private:
  bool is_alive; // true if alive
  unsigned int delay;
  double loss_prob;
  Node * node1;
  Node * node2;
  int cost;
};

#endif 
