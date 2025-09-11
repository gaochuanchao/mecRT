//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecIP2Nic.cc / MecIP2Nic.h
//
//  Description:
//    This file implements the IP to NIC interface. It extends the Simu5G IP2Nic module
//    to ensure data can be tranferred from the NIC module of an ES to its server module.
//    The original IP2Nic module in Simu5G does not allow data to be transferred to an unit
//    within the 5G core network.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef __MECRT_IP2NIC_H_
#define __MECRT_IP2NIC_H_

#include "stack/ip2nic/IP2Nic.h"

class MecIP2Nic : public IP2Nic
{
  protected:
    bool enableInitDebug_ = false; // enable debug info during initialization
    RanNodeType nodeType_;      // node type: can be ENODEB, GNODEB, UE
    inet::L3Address gnbAddress_;  // the Ipv4 address of the gNB (the cellularNic IP address)

    virtual void initialize(int stage) override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;
    virtual void registerInterface();
};

#endif
