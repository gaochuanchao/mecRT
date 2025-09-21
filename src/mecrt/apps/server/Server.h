//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    Server.cc / Server.h
//
//  Description:
//    This file implements the server in the edge server (RSU) in MEC.
//    The server handles the service initialization and termination, maintains resource status,
//    and communicates with the scheduler and the 5G NIC module.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_RSUSERVER_H_
#define _MECRT_RSUSERVER_H_

#include <string.h>
#include <omnetpp.h>

#include <inet/common/INETDefs.h>
#include <inet/transportlayer/contract/udp/UdpSocket.h>
#include <inet/networklayer/common/L3AddressResolver.h>
#include "common/binder/Binder.h"
#include "inet/networklayer/ipv4/Ipv4Header_m.h"
#include "inet/transportlayer/udp/UdpHeader_m.h"

#include "mecrt/common/MecCommon.h"
#include "mecrt/packets/apps/Grant2Rsu_m.h"
#include "mecrt/common/Database.h"
#include "mecrt/packets/apps/VecPacket_m.h"

class GnbMac;

class Server : public omnetpp::cSimpleModule
{
    friend class GnbMac;

  protected:
    bool enableInitDebug_ = false;
    inet::UdpSocket socket;
    int localPort_;
    int schedulerPort_;
    inet::L3Address schedulerAddr_;
    inet::L3Address serverAddr_;

    int cmpUnitTotal_;  // the total computing units in the RSU
    int cmpUnitFree_;   // the remaining free computing units in the RSU  

    VecResourceType resourceType_;  // "GPU" or "CPU"
    VecDeviceType deviceType_;    // "RTX3090", "RTX1080Ti", ...

    // IP address of the gateway to the Internet
    // inet::L3Address gwAddress_;
    // the GTP protocol Port, specified in GtpUser.ned module
    // unsigned int tunnelPeerPort_;

    // MEC routing information
    unsigned int rtUserPort_; // the port used by MecRtUser module
    string cellularNicName_; // the name of the cellular NIC interface

    Binder *binder_;
    MacNodeId gnbId_;
    int nicInterfaceId_;  // the interface id of the NIC module
    int pppIfInterfaceId_; // the interface id of the PPP interface

    // a map store granted service on this RSU
    std::map<AppId, Service> grantedService_;
    std::set<AppId> appsWaitStop_; // the apps that received stop command during service initialization
    std::set<AppId> appsWaitMacInitFb_;  // the apps that are waiting for the MAC layer initialization feedback

    // service initialization time, need to be initialized during module initialization
    std::map<int, omnetpp::simtime_t> serviceInitTime_;

    omnetpp::cMessage * srvInitComplete_;  // flush the app pdu list
    std::vector<AppId> srvInInitVector_;  // the service that are still in initializing status
    std::map<AppId, omnetpp::simtime_t> srvInitCompleteTime_;  // the initialization complete time of the service

    omnetpp::simsignal_t meetDlPktSignal_;  // the number of packets that meet the deadline
    omnetpp::simsignal_t failedSrvDownSignal_;  // the number of failed packets due to service down
    omnetpp::simsignal_t missDlPktSignal_;  // the number of packets that miss the deadline

  public:

    Server();
    ~Server();

    virtual int getServerPort() { return localPort_;}

    virtual int getSocketId() { return socket.getSocketId(); }


  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    virtual void initialize(int stage) override;

    virtual void initServiceStartingTime(int minTime, int maxTime);

    virtual void handleMessage(omnetpp::cMessage *msg) override;

    virtual void finish() override;

    virtual void updateRsuFeedback(omnetpp::cMessage *msg);

    virtual void updateServiceStatus(omnetpp::cMessage *msg);

    virtual void sendGrant2Vehicle(AppId appId, bool isStop);

    virtual void initializeService(inet::Ptr<const Grant2Rsu> pkt);

    virtual void stopService(AppId appId);

    virtual void addHeaders(inet::Packet *pkt, const inet::L3Address& destAddr, int destPort, const inet::Ipv4Address& srcAddr);

};
#endif
