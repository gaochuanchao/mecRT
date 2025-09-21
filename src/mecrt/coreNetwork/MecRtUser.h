//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecRtUser.cc / MecRtUser.h
//
//  Description:
//    This file implements a simple module that routes packets from the gnb to the user device.
//    When data is routing within the backhaul network, the data is first sent to this module of a gnb, then
//    it is forwarded to the user device by this module.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef __MECRT_MEC_RT_USER_H_
#define __MECRT_MEC_RT_USER_H_

#include <omnetpp.h>
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include <inet/linklayer/common/InterfaceTag_m.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include "common/binder/Binder.h"

class MecRtUser : public omnetpp::cSimpleModule
{
  protected:
    inet::UdpSocket socket_;
    int localPort_;
    inet::NetworkInterface* ie_;

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    // receive a GTP-U packet from Udp, reads the TEID and decides whether performing label switching or removal
    void handleFromUdp(inet::Packet * gtpMsg);

    // detect outgoing interface name (CellularNic)
    inet::NetworkInterface *detectInterface();
};


#endif
