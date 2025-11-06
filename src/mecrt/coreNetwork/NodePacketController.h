//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    NodePacketController.cc / NodePacketController.h
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

#ifndef __MECRT_NODE_PACKET_CONTROLLER_H_
#define __MECRT_NODE_PACKET_CONTROLLER_H_

#include <omnetpp.h>
#include "inet/transportlayer/contract/udp/UdpSocket.h"
#include <inet/linklayer/common/InterfaceTag_m.h>
#include "inet/common/socket/SocketTag_m.h"
#include <inet/networklayer/common/NetworkInterface.h>
#include "common/binder/Binder.h"

#include "mecrt/common/NodeInfo.h"
#include "mecrt/packets/apps/VecPacket_m.h"


using namespace std;
using namespace omnetpp;
using namespace inet;

class NodePacketController : public omnetpp::cSimpleModule
{
  protected:
    UdpSocket socket_;
    int socketId_;
    int localPort_;
    NodeInfo *nodeInfo_; // node information module
    bool enableInitDebug_ = false;

    // =========== UE application related ===========
    map<AppId, Ptr<VecRequest>> srvReqsBuffer_; // service requests buffer for global scheduler recovery
    map<AppId, set<int>> broadcastedSrvReqs_; // set of AppIds that have been broadcasted to other nodes

  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(omnetpp::cMessage *msg) override;

    virtual void handleServiceRequest(inet::Packet *packet);
    virtual void handleServiceGrant(inet::Packet *packet);
    virtual void handleServiceFeedback(inet::Packet *packet);
    virtual void handleOffloadingNicGrant(inet::Packet *packet);

  public:
    NodePacketController();
    virtual ~NodePacketController();

    virtual void recoverServiceRequests();
};


#endif
