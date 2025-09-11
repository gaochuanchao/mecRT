//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecGtpUser.cc / MecGtpUser.h
//
//  Description:
//    This file implements the GTP user functionality in the MEC system.
//    The MecGtpUser is responsible for handling the communication between
//    the RSU and the scheduler, encapsulating IP datagrams in GTP-U packets
//    and managing the data tunnels between GTP peers.
//
//  Original code from Simu5G (https://github.com/Unipisa/Simu5G/blob/master/src/corenetwork/gtp/GtpUser.h)
//
//  Adjustment:
//    - Ensuring data packets routing between ES and scheduler via GTP tunneling.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef __MECRT_GTP_USER_H_
#define __MECRT_GTP_USER_H_

#include <map>
#include <omnetpp.h>
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include "inet/networklayer/common/L3AddressResolver.h"
#include "corenetwork/gtp/GtpUserMsg_m.h"
#include <inet/common/ModuleAccess.h>
#include <inet/networklayer/common/NetworkInterface.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "common/binder/Binder.h"
#include <inet/linklayer/common/InterfaceTag_m.h>

/**
 * GtpUser is used for building data tunnels between GTP peers.
 * GtpUser can receive two kind of packets:
 * a) IP datagram from a trafficFilter. Those packets are labeled with a tftId
 * b) GtpUserMsg from Udp-IP layers.
 * MecGtpUser is an extension of GtpUser for handling the communication between RSU and scheduler.
 */

class MecGtpUser : public omnetpp::cSimpleModule
{
    inet::UdpSocket socket_;
    int localPort_;

    int pppIfInterfaceId_; // the interface id of the PPP interface

    // reference to the LTE Binder module
    Binder* binder_;

    // the GTP protocol Port
    unsigned int tunnelPeerPort_;

    // IP address of the gateway to the Internet
    inet::L3Address gwAddress_;

    // specifies the type of the node that contains this filter (it can be ENB or PGW)
    CoreNodeType ownerType_;

    CoreNodeType selectOwnerType(const char * type);

    // if this module is on BS, this variable includes the ID of the BS
    MacNodeId myMacNodeID;

    inet::NetworkInterface* ie_;

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    // receive and IP Datagram from the traffic filter, encapsulates it in a GTP-U packet than forwards it to the proper next hop
    void handleFromTrafficFlowFilter(inet::Packet * datagram);

    // receive a GTP-U packet from Udp, reads the TEID and decides whether performing label switching or removal
    void handleFromUdp(inet::Packet * gtpMsg);

    // detect outgoing interface name (CellularNic)
    inet::NetworkInterface *detectInterface();
};

#endif
