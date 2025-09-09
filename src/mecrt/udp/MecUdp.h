//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
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