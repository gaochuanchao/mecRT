//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecUdp.cc / MecUdp.h
//
//  Description:
//    This file implements the UDP protocol with MEC support.
//    It extends the INET Udp module to handle large data packets.
//    The original Udp module can only handle packets up to 65527 Bytes, and fragmentation will occur for larger packets.
//    The fragmentation process significantly slows down the simulation.
//    To accelerate the simulation, we make the packet size threshold configurable, 
//    allowing larger packets to be sent without fragmentation.
//
//    Note that in real networks, packets larger than the MTU will be fragmented.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef __MECRT_UDP_H
#define __MECRT_UDP_H

#include "inet/transportlayer/udp/Udp.h"

using namespace inet;

class MecUdp : public Udp
{
  protected:
    int mtu_;  // maximum transmission unit

    virtual void initialize(int stage) override;
    // process packets from application
    virtual void handleUpperPacket(Packet *appData) override;
    
};



#endif