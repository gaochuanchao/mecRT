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

using namespace inet;

class NodePacketController;
class GnbMac;

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
        inet::Ipv4Address nodeAddr_; // [MecOspf, UeMac] IPv4 address of the ipv4 module in this node
        bool rtState_; // [MecOspf] whether the routing table has been set up by the routing protocol (e.g., OSPF)
        int npcSocketId_; // [NodePacketController] the socket id used by the NodePacketController module

        // =========== Wireless NIC module related ===========
        MacNodeId nodeId_;    // [GnbMac, UeMac] macNodeId of the wireless NIC
        int nicInterfaceId_;  // [MecIP2Nic] the interface id of the NIC module
        bool nicState_; // whether the NIC is active (true) or inactive (false), used for fault simulation

        // =========== Server module related ===========
        bool serverState_; // whether the server is active (true) or inactive (false), used for fault simulation
        int serverPort_; // [Server] the port number used by the server module
        int serverSocketId_; // [Server] the socket id used by the server module

        // =========== Scheduler module related ===========
        bool isGlobalScheduler_; // [MecOspf] whether this scheduler is voted as the global scheduler
        inet::Ipv4Address globalSchedulerAddr_; // [MecOspf] the IPv4 address of the selected scheduler node
        int localSchedulerPort_; // [Scheduler] the port number used by the local scheduler module
        int localSchedulerSocketId_; // [Scheduler] the socket id used by the local scheduler module
        double scheduleInterval_; // in seconds

        // =========== reference to other modules ===========
        GnbMac* gnbMac_ = nullptr;
        NodePacketController* npc_ = nullptr;

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
        void setNodeAddr(inet::Ipv4Address addr) {nodeAddr_ = addr;}
        inet::Ipv4Address getNodeAddr() {return nodeAddr_;}
        void setRtState(bool state) {rtState_ = state;}
        bool getRtState() {return rtState_;}
        bool isRtReady() {return rtState_;}

        void setNpcSocketId(int id) {npcSocketId_ = id;}
        int getNpcSocketId() {return npcSocketId_;}

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
        void setServerSocketId(int id) {serverSocketId_ = id;}
        int getServerSocketId() {return serverSocketId_;}

        // methods to set/get scheduler module related information
        void setIsGlobalScheduler(bool isGlobal) {isGlobalScheduler_ = isGlobal;}
        bool getIsGlobalScheduler() {return isGlobalScheduler_;}
        void setGlobalSchedulerAddr(inet::Ipv4Address addr) {globalSchedulerAddr_ = addr;}
        inet::Ipv4Address getGlobalSchedulerAddr() {return globalSchedulerAddr_;}
        void setLocalSchedulerPort(int port) {localSchedulerPort_ = port;}
        int getLocalSchedulerPort() {return localSchedulerPort_;}
        void setScheduleInterval(double interval) {scheduleInterval_ = interval;}
        double getScheduleInterval() {return scheduleInterval_;}
        void setLocalSchedulerSocketId(int id) {localSchedulerSocketId_ = id;}
        int getLocalSchedulerSocketId() {return localSchedulerSocketId_;}


        // modules reference related methods
        void setGnbMac(GnbMac* mac) {gnbMac_ = mac;}
        virtual void recoverRsuStatus();
        void setNpc(NodePacketController* npc) {npc_ = npc;}
        virtual void recoverServiceRequests();
};

#endif /* _MECRT_COMMON_NODEINFO_H_ */



/***
 * ============== Parameters Initialization Sequence ==============
 * // =========== Routing related ===========
 * inet::Ipv4Address nodeAddr_;
 *      - set: MecOspf, INITSTAGE_NETWORK_LAYER
 *      - set: UeMac, INITSTAGE_NETWORK_LAYER
 *      - get: GnbMac, INITSTAGE_LAST
 * int npcSocketId_;
 *      - set: NodePacketController, INITSTAGE_APPLICATION_LAYER
 *
 * // =========== Wireless NIC module related ===========
 * MacNodeId nodeId_;
 *      - set: UeMac (INITSTAGE_LINK_LAYER)
 *      - set: GnbMac (INITSTAGE_PHYSICAL_ENVIRONMENT)
 *      - get: MecOspf, INITSTAGE_ROUTING_PROTOCOLS
 *      - get: UeApp, INITSTAGE_LAST
 * int nicInterfaceId_;
 *      - set: MecIP2Nic, INITSTAGE_PHYSICAL_ENVIRONMENT
 * 
 * // =========== Server module related ===========
 * int serverPort_;
 *      - set: Server, INITSTAGE_APPLICATION_LAYER
 *      - get: GnbMac, INITSTAGE_LAST
 * int serverSocketId_;
 *      - set: Server, INITSTAGE_APPLICATION_LAYER
 *
 * // =========== Scheduler module related ===========
 * int localSchedulerPort_;
 *    - set: Scheduler, INITSTAGE_APPLICATION_LAYER
 * int localSchedulerSocketId_;
 *    - set: Scheduler, INITSTAGE_APPLICATION_LAYER
 * double scheduleInterval_; // in seconds
 *    - set: Scheduler, INITSTAGE_APPLICATION_LAYER
 */
