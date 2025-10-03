//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecCommon.h/MecCommon.cc
//
//  Description:
//    Collect metadata for different submodules of this node,
//    make it easier to fetch the information from different modules within the same node.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-16
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_COMMON_NODEINFO_H_
#define _MECRT_COMMON_NODEINFO_H_


#include <omnetpp.h>
#include <inet/common/INETDefs.h>
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"
#include "mecrt/common/MecCommon.h"

using namespace omnetpp;
using namespace inet;

/**
 * @class NodeInfo
 * @brief There is one NodeInfo module for each node. Keeps cross-layer information about the node
 */
class NodeInfo : public omnetpp::cSimpleModule
{
    protected:
        bool enableInitDebug_ = false;

        // =========== Basic node information ===========
        std::string nodeType_;
        bool nodeState_; // whether this node is active (true) or inactive (false), used for fault simulation

        // =========== Routing related ===========
        Ipv4Address nodeAddr_; // [MecOspf] IPv4 address of the ipv4 module in this node
        int npcPort_; // [NodePacketController] the port used by the Node Packet Controller (Npc) module
        bool rtState_; // [MecOspf] whether the routing table has been set up by the routing protocol (e.g., OSPF)

        // =========== Wireless NIC module related ===========
        MacNodeId nodeId_;    // [GnbMac] macNodeId of the wireless NIC
        int nicInterfaceId_;  // [MecIP2Nic] the interface id of the NIC module
        bool nicState_; // whether the NIC is active (true) or inactive (false), used for fault simulation

        // =========== Server module related ===========
        bool serverState_; // whether the server is active (true) or inactive (false), used for fault simulation
        int serverPort_; // [Server] the port number used by the server module

        // =========== Scheduler module related ===========
        bool isGlobalScheduler_; // [MecOspf] whether this scheduler is voted as the global scheduler
        Ipv4Address globalSchedulerAddr_; // [MecOspf] the IPv4 address of the selected scheduler node
        int localSchedulerPort_; //  the port number used by the local scheduler module
        double scheduleInterval_; // in seconds

    protected:
        virtual void initialize(int stage) override;
        virtual int numInitStages() const override { return NUM_INIT_STAGES; }

    public:
        NodeInfo();
        virtual ~NodeInfo() {};
        
        // methods to set/get basic node information
        void setNodeType(std::string nodeType) {nodeType_ = nodeType;}
        std::string getNodeType() {return nodeType_;}
        void setNodeState(bool state) {nodeState_ = state;}
        bool getNodeState() {return nodeState_;}
        bool isNodeActive() {return nodeState_;}

        // methods to set/get routing related information
        void setNodeAddr(Ipv4Address addr) {nodeAddr_ = addr;}
        Ipv4Address getNodeAddr() {return nodeAddr_;}
        void setNpcPort(int port) {npcPort_ = port;}
        int getNpcPort() {return npcPort_;}
        void setRtState(bool state) {rtState_ = state;}
        bool getRtState() {return rtState_;}
        bool isRtReady() {return rtState_;}

        // methods to set/get wireless NIC module related information
        void setNodeId(MacNodeId id) {nodeId_ = id;}
        MacNodeId getNodeId() {return nodeId_;}
        void setNicInterfaceId(int id) {nicInterfaceId_ = id;}
        int getNicInterfaceId() {return nicInterfaceId_;}
        void setNicState(bool state) {nicState_ = state;}
        bool getNicState() {return nicState_;}
        bool isNicActive() {return nicState_;}

        // methods to set/get server module related information
        void setServerState(bool state) {serverState_ = state;}
        bool getServerState() {return serverState_;}
        bool isServerActive() {return serverState_;}
        void setServerPort(int port) {serverPort_ = port;}
        int getServerPort() {return serverPort_;}

        // methods to set/get scheduler module related information
        void setIsGlobalScheduler(bool isGlobal) {isGlobalScheduler_ = isGlobal;}
        bool getIsGlobalScheduler() {return isGlobalScheduler_;}
        void setGlobalSchedulerAddr(Ipv4Address addr) {globalSchedulerAddr_ = addr;}
        Ipv4Address getGlobalSchedulerAddr() {return globalSchedulerAddr_;}
        void setLocalSchedulerPort(int port) {localSchedulerPort_ = port;}
        int getLocalSchedulerPort() {return localSchedulerPort_;}
        void setScheduleInterval(double interval) {scheduleInterval_ = interval;}
        double getScheduleInterval() {return scheduleInterval_;}
};

#endif /* _MECRT_COMMON_NODEINFO_H_ */
