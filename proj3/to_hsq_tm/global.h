#ifndef GLOB_H
#define GLOB_H

#include <vector>
#include <queue>
#include <fstream>
#include <string>
#include <unordered_map>
#include <cstdlib>
#include <cstdio>

#include <iostream>


using namespace std;
using namespace __gnu_cxx;

#define DEFAULT_DELAY 0.1
#define DEFAULT_PROB 0.8
#define DEFAULT_COST 1

enum eEventType {
        EVENT_LINK_DYING,
        EVENT_LINK_COMINGUP,
        EVENT_XMIT_PKT,
        EVENT_NEW_LINK
};

/*************************************/
/* BEGIN constants students must use */
/*************************************/

#define INFINITY_COST 0xffff
// This should be used to represent infinity cost in DV update
// messages when poison reverse is applied. Link cost is the same as
// round-trip-time in milliseconds, which is represented as an
// unsigned short (16 bit) in DV update packets

#define SPECIAL_PORT 0xffff
// When a packet is received from this port number, it means the packet
// is originating locally rather than received from a neighbor

enum ePacketType {
  DATA,
  PING,
  PONG,
  DV,
  LS,
};

enum eProtocolType {
  P_DV,
  P_LS
};

/*************************************/
/* END constants students must use   */
/*************************************/


#endif
