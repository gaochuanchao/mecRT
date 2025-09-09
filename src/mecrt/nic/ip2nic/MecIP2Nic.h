//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// in order to send data to the APP (the RSU server)
//

#ifndef __MECRT_IP2NIC_H_
#define __MECRT_IP2NIC_H_

#include "stack/ip2nic/IP2Nic.h"

class MecIP2Nic : public IP2Nic
{
  protected:
    RanNodeType nodeType_;      // node type: can be ENODEB, GNODEB, UE
    inet::L3Address gnbAddress_;  // the Ipv4 address of the gNB (the cellularNic IP address)

    virtual void initialize(int stage) override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void registerInterface();
};

#endif
