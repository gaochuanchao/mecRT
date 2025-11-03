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
#include "mecrt/common/NodeInfo.h"

using namespace inet;

struct Service
{
    AppId appId;    // the application id
    inet::Ipv4Address ueAddr; // the IP address of the vehicle
    string resourceType;  // whether using GPU or CPU
    string service;        // the service name
    MacNodeId processGnbId;  // the id of processing gNB
    MacNodeId offloadGnbId;  // the id of offloading gNB
    inet::Ipv4Address offloadGnbAddr; // the IP address of offloading gNB
    omnetpp::simtime_t exeTime;    // service execution time
    int cmpUnits;        // the allocated computing units for the service in RSU
    int bands;           // the allocated bands for the service in RSU
    omnetpp::simtime_t deadline;  // the deadline of the service
    int inputSize;  // the input data size of the job, in bytes
    int outputSize;  // the output data size of the job, in bytes
    bool initComplete;
    omnetpp::simtime_t maxOffloadTime;  // the maximum offloading time results in positive energy saving
    double utility; // the utility of the service instance per second
};

class NodeInfo;

class Server : public omnetpp::cSimpleModule
{
    friend class NodeInfo;

  protected:
    bool enableInitDebug_ = false;
    inet::UdpSocket socket;
    int socketId_;
    int localPort_;

    NodeInfo *nodeInfo_ = nullptr;

    int cmpUnitTotal_;  // the total computing units in the RSU
    int cmpUnitFree_;   // the remaining free computing units in the RSU  

    string resourceType_;  // "GPU" or "CPU"
    string deviceType_;    // "RTX3090", "RTX1080Ti", ...

    // IP address of the gateway to the Internet
    // inet::L3Address gwAddress_;
    // the GTP protocol Port, specified in GtpUser.ned module
    // unsigned int tunnelPeerPort_;

    // MEC routing information
    string cellularNicName_; // the name of the cellular NIC interface

    Database *db_;
    Binder *binder_;
    MacNodeId gnbId_;
    // int pppIfInterfaceId_; // the interface id of the PPP interface

    // a map store granted service on this RSU
    std::map<AppId, Service> grantedService_;
    std::set<AppId> appsWaitStop_; // the apps that received stop command during service initialization
    std::set<AppId> appsWaitMacInitFb_;  // the apps that are waiting for the MAC layer initialization feedback

    // service initialization time, need to be initialized during module initialization
    std::map<string, omnetpp::simtime_t> serviceInitTime_;

    omnetpp::cMessage * srvInitComplete_;  // flush the app pdu list
    std::vector<AppId> srvInInitVector_;  // the service that are still in initializing status
    std::map<AppId, omnetpp::simtime_t> srvInitCompleteTime_;  // the initialization complete time of the service

    omnetpp::cMessage * appDataReceivedTimer_; // timer to process the received app data packets
    int receivedDataCount_; // the number of received data packets during the timer interval
    double receivedDataUtility_; // the total utility of received data packets during the timer interval

    omnetpp::simsignal_t meetDlPktSignal_;  // the number of packets that meet the deadline
    omnetpp::simsignal_t failedSrvDownSignal_;  // the number of failed packets due to service down
    omnetpp::simsignal_t missDlPktSignal_;  // the number of packets that miss the deadline
    omnetpp::simsignal_t utilitySignal_;  // the utility of the service instance per second

  public:

    Server();
    ~Server();

    virtual int getServerPort() { return localPort_;}

    virtual int getSocketId() { return socket.getSocketId(); }

    virtual void releaseServerResources();


  protected:

    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    virtual void initialize(int stage) override;

    virtual void initServiceStartingTime(int minTime, int maxTime);

    virtual void handleMessage(omnetpp::cMessage *msg) override;

    virtual void handleServiceInitComplete();

    virtual void handleAppData(inet::Packet *pkt);

    virtual void finish() override;

    virtual void handleRsuFeedback(inet::Packet *pkt);

    virtual void handleSeviceFeedback(omnetpp::cMessage *msg);

    virtual void sendGrant2OffloadingNic(AppId appId, bool isStop);

    virtual void initializeService(inet::Ptr<const Grant2Rsu> pkt);

    virtual void stopService(AppId appId);

};
#endif
