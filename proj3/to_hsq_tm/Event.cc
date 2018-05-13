#include "Event.h"

#include "Simulator.h"

const char *sPacketType[] = {"DATA","PING","PONG","DV","LS"};

extern Simulator * sim;

void Event_Change_Delay::dispatch() {
  link->change_delay(delay);
}

void Event_Link_Die::dispatch() {
  dying_link->link_dies();
}

void Event_Link_Come_Up::dispatch() {
  upcoming_link->link_comes_up();
}

void Event_Xmit_Pkt_End_To_End_Generic::dispatch() {
  src->recv_packet(SPECIAL_PORT, pkt, size); // SPECIAL_PORT means pkt is locally generated
}

void Event_Xmit_Pkt_On_Link_Generic::dispatch() {
  //  cout<<"Delay is "<<ln->get_delay()<<endl;

  if (ln->get_is_alive() && (rand()%10 >= (10*ln->get_loss_prob()))) {

    Event * recv_event = new Event_Recv_Pkt_Generic(dest, dport, pkt, size, time + ln->get_delay());
    sim->event_q.push(recv_event);
  
  } else {
    // no transmission, packet lost
    cout<<"Packet lost\n";
    free(pkt);
  }
}

void Event_Xmit_Pkt_On_Link_Generic:: pt() {
  unsigned int t;

  t = *((unsigned char *)pkt);

  cout<<"Event_Xmit_Pkt_On_Link (";
  if (dest == ln->get_node1()) {
    cout<<ln->get_node2()->id<<","<<ln->get_node1()->id;
  } else {
    cout<<ln->get_node1()->id<<","<<ln->get_node2()->id;
  }
  cout<<") packet type is "<<sPacketType[t]<<"\n";
}

void Event_Recv_Pkt_Generic::dispatch() {
  dest->recv_packet(dport, pkt, size);
}

void Event_Recv_Pkt_Generic:: pt() {
  unsigned int t;

  t = *((unsigned char *)pkt);

  cout<<"Event_Recv_Pkt_On_Node "<<dest->id<<" packet type is "<<sPacketType[t]<<"\n";
}  

void Event_Alarm::dispatch() {
  rp->handle_alarm(data);
}

