//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecOspf.h / MecOspf.cc
//
//  Description:
//    This file implements the dynamic routing functionality in the edge server (RSU) in MEC.
//    The router handles neighbor discovery, topology management, and dynamic routing table updates.
//
//      1. Interface management
//         - Detect neighbors dynamically (like OSPF Hello packets).
//         - Track interface states (up/down).
//      2. Neighbor table
//         - Keep track of neighbor IPs, interface indices, and last-seen times.
//      3. Topology graph
//         - Maintain a representation of the network graph (nodes + links).
//         - Use this graph to recompute shortest paths on link/node failures.
//      4. Routing table updates
//         - Integrate with Ipv4RoutingTable dynamically.
//         - Use the shortest-path computation to fill next-hop entries.
//      5. Failure handling
//         - Detect link/node failures.
//         - Remove affected routes and recompute paths.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/routing/MecOspf.h"
#include "mecrt/packets/routing/OspfHello_m.h"
#include "mecrt/common/MecCommon.h"

#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/common/NetworkInterface.h"

#include <inet/networklayer/ipv4/Ipv4Header_m.h>
#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include <inet/linklayer/common/InterfaceTag_m.h>
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/common/TagBase_m.h"
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/packet/Packet.h"
#include "inet/common/packet/chunk/ByteCountChunk.h"
#include <queue>

Define_Module(MecOspf);


MecOspf::MecOspf()
{
    helloTimer_ = nullptr;
    lsaTimer_ = nullptr;
    routeComputationTimer_ = nullptr;

    ift_ = nullptr;
    rt_ = nullptr;
    selfLsa_ = nullptr;
    nodeInfo_ = nullptr;

    indirectRoutes_ = map<uint32_t, Ipv4Route *>();
    neighborRoutes_ = map<uint32_t, Ipv4Route *>();
}

MecOspf::~MecOspf()
{
    if (enableInitDebug_)
        std::cout << "MecOspf::~MecOspf - destroying OSPF protocol\n";

    if (helloTimer_) {
        cancelAndDelete(helloTimer_);
        helloTimer_ = nullptr;
    }

    if (lsaTimer_) {
        cancelAndDelete(lsaTimer_);
        lsaTimer_ = nullptr;
    }

    if (routeComputationTimer_) {
        cancelAndDelete(routeComputationTimer_);
        routeComputationTimer_ = nullptr;
    }

    if (enableInitDebug_)
        std::cout << "MecOspf::~MecOspf - cleaning up OSPF protocol done!\n";
}

void MecOspf::initialize(int stage)
{
    cSimpleModule::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "MecOspf::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

        EV_INFO << "MecOspf::initialize - stage: INITSTAGE_LOCAL - begins\n";
        // registerMecOspfProtocol(); // ensure protocol is registered before simulation starts

        neighborChanged_ = false;
        // create timers
        helloTimer_ = new cMessage("helloTimer");
        helloTimer_->setSchedulingPriority(1); // after other messages
        lsaTimer_ = new cMessage("lsaTimer");
        lsaTimer_->setSchedulingPriority(1); // after other messages
        routeComputationTimer_ = new cMessage("routeComputationTimer");
        routeComputationTimer_->setSchedulingPriority(1); // after other messages

        lsaWaitInterval_ = par("lsaWaitInterval").doubleValue();
        helloInterval_ = par("helloInterval").doubleValue();
        neighborTimeout_ = 2 * helloInterval_; // set dead interval as 2*helloInterval
        routeComputationDelay_ = par("routeComputationDelay").doubleValue();

        simtime_t startupTime = par("startupTime");
        scheduleAfter(startupTime, helloTimer_);

        WATCH(lsaWaitInterval_);
        WATCH(helloInterval_);
        WATCH(neighborTimeout_);
        WATCH(routeComputationDelay_);

        if (enableInitDebug_)
            cout << "MecOspf:initialize - stage: INITSTAGE_LOCAL - done\n";
    }
    else if (stage == INITSTAGE_PHYSICAL_LAYER) {
        // get node info module, as router may not constains this module, so we need to handle the exception
        if (enableInitDebug_)
            cout << "MecOspf:initialize - stage: INITSTAGE_PHYSICAL_LAYER - begins\n";
        
        try {
            nodeInfo_ = getModuleFromPar<NodeInfo>(par("nodeInfoModulePath"), this);
        } catch (cException &e) {
            cerr << "MecOspf:initialize - cannot find nodeInfo module\n";
            nodeInfo_ = nullptr;
        }

        if (enableInitDebug_)
            cout << "MecOspf:initialize - stage: INITSTAGE_PHYSICAL_LAYER - nodeInfo_ found: " 
                    << (nodeInfo_ ? "yes" : "no") << "\n";
    }
    else if (stage == INITSTAGE_NETWORK_LAYER)
    {
        if (enableInitDebug_)
            cout << "MecOspf:initialize - stage: INITSTAGE_NETWORK_LAYER - begins\n";

        try {
            rt_ = getModuleFromPar<Ipv4RoutingTable>(par("routingTableModule"), this);
            routerId_ = rt_->getRouterId();
            routerIdKey_ = routerId_.getInt();

            if (nodeInfo_)
                nodeInfo_->setNodeAddr(routerId_);

            EV_INFO << "MecOspf:initialize - routingTableModule found, routerId=" << routerId_ << "\n";
        } catch (cException &e) {
            throw cRuntimeError("MecOspf:initialize - cannot find routingTableModule param\n");
        }

        // nothing to do here, just for logging purpose
        if (enableInitDebug_)
            cout << "MecOspf:initialize - stage: INITSTAGE_NETWORK_LAYER - routerId=" << routerId_ << "\n";
    }
    else if (stage == INITSTAGE_ROUTING_PROTOCOLS) {
        if (enableInitDebug_)
            cout << "MecOspf:initialize - INITSTAGE_ROUTING_PROTOCOLS stage " << stage << "\n";

        EV_INFO << "MecOspf:initialize - INITSTAGE_ROUTING_PROTOCOLS stage " << stage << "\n";
        // acquire required INET modules via parameters (StandardHost uses these params)
        try {
            ift_ = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

            // make sure our module joins the OSPF multicast groups on all interfaces
            // (otherwise we won't receive OSPF multicast packets)
            // for (int i = 0; i < ift_->getNumInterfaces(); i++) {
            //     NetworkInterface *ie = ift_->getInterface(i);
            //     if (ie->isLoopback()) continue; // skip loopback interface
            //     ie->joinMulticastGroup(Ipv4Address::ALL_OSPF_ROUTERS_MCAST);
            //     ie->joinMulticastGroup(Ipv4Address::ALL_OSPF_DESIGNATED_ROUTERS_MCAST);
            // }

            if (nodeInfo_)
            {
                nodeInfo_->setIft(ift_);
                nodeInfo_->setOspf(this);
            }
                
            EV_INFO << "MecOspf:initialize - interfaceTableModule found\n";
        } catch (cException &e) {
            throw cRuntimeError("MecOspf:initialize - cannot find interfaceTableModule param\n");
        }

        // register protocol such that the IP layer can deliver packets to us
        // registerProtocol(*MecProtocol::mecOspf, gate("ipOut"), gate("ipIn"));

        EV_INFO << "MecOspf:initialize - INITSTAGE_ROUTING_PROTOCOLS init. Interfaces=" << (ift_ ? ift_->getNumInterfaces() : 0)
                << " RoutingTable=" << (rt_ ? "found" : "NOT_FOUND") << "\n";

        // initialize its Link State Advertisement (LSA) info
        seqNum_ = 0;
        selfLsa_ = makeShared<OspfLsa>();
        selfLsa_->setOrigin(routerIdKey_);
        selfLsa_->setSeqNum(seqNum_);
        selfLsa_->setInstallTime(simTime());
        selfLsa_->setNodeId(nodeInfo_ ? nodeInfo_->getNodeId() : -1);   // set nodeId if nodeInfo_ is available (gNB), otherwise -1
        lsaPacketCache_[routerIdKey_] = selfLsa_;
        ipv4ToMacNodeId_[routerIdKey_] = selfLsa_->getNodeId();

        WATCH(routerId_);
        WATCH(routerIdKey_);
        WATCH(neighborChanged_);
        WATCH(seqNum_);
        WATCH(schedulerAddr_);
        WATCH_VECTOR(newNeighbors_);
        WATCH_MAP(indirectRoutes_);
        WATCH_MAP(neighborRoutes_);
        WATCH_MAP(lsaPacketCache_);

        if (enableInitDebug_)
            cout << "MecOspf:initialize - stage: INITSTAGE_ROUTING_PROTOCOLS - done\n";
    }
    else if (stage == INITSTAGE_APPLICATION_LAYER)
    {
        if (enableInitDebug_)
            cout << "MecOspf:initialize - stage: INITSTAGE_APPLICATION_LAYER - begins\n";

        // define local port and bind socket
        localPort_ = MEC_OSPF_PORT; // fixed port for OSPF
        socket_.setOutputGate(gate("socketOut"));
        socket_.bind(localPort_);
        socketId_ = socket_.getSocketId();
        socket_.joinMulticastGroup(Ipv4Address::ALL_OSPF_ROUTERS_MCAST);

        EV_INFO << "MecOspf::initialize - stage: INITSTAGE_APPLICATION_LAYER - bound to port: local:" << localPort_ << "\n";

        WATCH(localPort_);
        WATCH(socketId_);

        if (enableInitDebug_)
            cout << "MecOspf::initialize - stage: INITSTAGE_APPLICATION_LAYER - done\n";
    }
}

// central message dispatcher
void MecOspf::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // timer fired
        EV_INFO << "MecOspf:handleMessage - self-message " << msg->getName() << "\n";
        handleSelfTimer(msg);
        return;
    }
    else
    {
        Packet *packet = check_and_cast<Packet *>(msg);
        const char *pname = packet->getName();

        // check arrival interface
        NetworkInterface *arrivalIf = ift_->getInterfaceById(packet->findTag<InterfaceInd>()->getInterfaceId());
        if (arrivalIf->isDown()) {
            EV_WARN << NOW << " MecOspf:handleMessage - received packet on down interface " << arrivalIf->getInterfaceName() << ", delete it!\n";
            delete packet;
            packet = nullptr;
            return;
        }

        // process based on packet type
        if (pname && strcmp(pname, "OspfHello") == 0) {
            EV_INFO << "MecOspf:handleMessage - OspfHello received\n";
            processHello(packet);

            delete packet;
            packet = nullptr;
            return;
        }
        else if (pname && strcmp(pname, "OspfLsa") == 0) {
            EV_INFO << "MecOspf:handleMessage - OspfLsa received\n";
            handleReceivedLsa(packet);

            delete packet;
            packet = nullptr;
            return;
        }
        else
        {
            // Otherwise treat as data packet to forward
            EV_WARN << "MecOspf:handleMessage - data packet received: " << (pname ? pname : "(unnamed)") << ", delete!\n";
            delete packet;
            packet = nullptr;
            return;
        }
    }
}


/* === handleTimer ===
 * Called when helloTimer_ fires. Send Hello, check neighbor timeouts, reschedule.
 */
void MecOspf::handleSelfTimer(cMessage *msg)
{
    if (msg == helloTimer_)
    {
        EV_INFO << "MecOspf::handleSelfTimer - Hello timer fired at " << simTime() << "\n";

        // reschedule next Hello, continuous monitor neighbor accessibility
        scheduleAt(simTime() + helloInterval_, helloTimer_);

        if (nodeInfo_ && !nodeInfo_->isNodeActive())    // node has become inactive, skip sending Hello
            return;

        newNeighbors_.clear();
        // send Hellos
        sendInitialHello();
        
        // also schedule LSA synchronization timer
        if (lsaTimer_->isScheduled())
            cancelEvent(lsaTimer_);

        scheduleAt(simTime() + lsaWaitInterval_, lsaTimer_);
    }
    else if (msg == lsaTimer_)
    {
        EV_INFO << "MecOspf::handleSelfTimer - LSA timer fired at " << simTime() << "\n";

        // detect neighbor timeouts (link failure)
        checkNeighborTimeouts();

        // update LSA if needed
        handleLsaTimer();
    }
    else if (msg == routeComputationTimer_)
    {
        EV_INFO << "MecOspf::handleSelfTimer - Route Computation timer fired at " << simTime() << "\n";

        // recompute routes to all nodes in the topology
        recomputeIndirectRouting();
    }
    else
    {
        EV_WARN << "MecOspf::handleSelfTimer - unknown self-message: " << msg->getName() << "\n";
    }
}


void MecOspf::handleLsaTimer()
{
    // if neighbor change happened, send LSA to neighbors
    if (!neighborChanged_) {
        EV << "MecOspf:handleSelfTimer - no neighbor change detected, skip LSA update\n";
        return;
    }

    EV << "MecOspf:handleSelfTimer - neighbor change detected, updating LSA and sending to network\n";

    seqNum_++;

    // update our own LSA packet cache
    selfLsa_->setSeqNum(seqNum_);
    simtime_t installTime = simTime();
    selfLsa_->setInstallTime(installTime);
    // reset and add neighbor list to LSA chunk
    int neighborCount = neighbors_.size();
    selfLsa_->setNeighborArraySize(neighborCount);
    selfLsa_->setCostArraySize(neighborCount);
    int idx = 0;
    for (const auto& n : neighbors_) {
        selfLsa_->setNeighbor(idx, n.second.destIp.getInt());
        selfLsa_->setCost(idx, n.second.cost);
        idx++;
    }

    // send LSA to neighbors
    updateLsaToNetwork();

    // reset neighbor change flag and clear new neighbor list
    neighborChanged_ = false;
    newNeighbors_.clear();

    if (!routeComputationTimer_->isScheduled())
    {
        scheduleAt(installTime + routeComputationDelay_, routeComputationTimer_);
        largestLsaTime_ = installTime;

        EV << "MecOspf:handleSelfTimer - scheduled route recomputation at " << (installTime + routeComputationDelay_) << "\n";
    }
    else if (routeComputationTimer_->isScheduled() && largestLsaTime_ < installTime)
    {
        largestLsaTime_ = installTime;
        cancelEvent(routeComputationTimer_);
        scheduleAt(installTime + routeComputationDelay_, routeComputationTimer_);

        EV << "MecOspf:handleSelfTimer - rescheduled route recomputation at " << (installTime + routeComputationDelay_) << "\n";
    }
}


/* === sendHello ===
 * Create a Packet named "HELLO" and tag it with destination address and outgoing interface.
 * - Every router sends Hello packets periodically out each interface.
 * - If a neighbor responds (or its Hello arrives), they become “neighbors.”
 * - If no Hello is seen for a Dead Interval → neighbor considered lost.
 */
void MecOspf::sendInitialHello()
{
    if (!ift_) {
        EV_WARN << "MecOspf::sendInitialHello - no IInterfaceTable available\n";
        return;
    }

    // iterate all interfaces and send Hello where appropriate
    for (int i = 0; i < ift_->getNumInterfaces(); ++i) {
        NetworkInterface *ie = ift_->getInterface(i);
        if (!ie || ie->isLoopback() || !ie->isUp() || ie->isWireless()) // skip loopback/down/wireless interfaces
            continue;

        // get the IPv4 address for this interface
        Ipv4Address local = ie->getIpv4Address();
        if (local.isUnspecified()) continue;

        //TODO: the packet has print errors in ipv4 module in the gui mode, but works fine in cmdenv mode
        // this issue will cause a core dump when we close the GUI window during ospf synchronization
        // consider to move it to the application layer in the future

        // Build a Packet named OspfHello
        Packet *hello = new Packet("OspfHello");
        // Add a payload marker so it isn't empty (optional)
        auto ospfHelloChunk = makeShared<OspfHello>();
        ospfHelloChunk->setSenderIp(routerIdKey_);
        hello->insertAtBack(ospfHelloChunk);
        // Tag outgoing interface id (so ipOut knows which interface)
        hello->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());

        // optionally add a small custom tag or param to mark as Hello
        // (we already used packet name "HELLO", which we check when receiving)
        EV_INFO << "MecOspf:sendInitialHello - sending OspfHello from " << routerId_ << " (" << routerIdKey_ << ")"
                                    << " via interface " << ie->getInterfaceName() << "\n";

        // send(hello, "ipOut");
        socket_.sendTo(hello, Ipv4Address::ALL_OSPF_ROUTERS_MCAST, MEC_OSPF_PORT);
    }
}


// send Hello in response to received Hello (not used now)
// used for synchronization mutual discovery
void MecOspf::sendHelloFeedback(Packet *pkt)
{
    // extract neighbor ip and arriving interface from Hello chunk
    auto ospfHelloChunk = pkt->peekAtFront<OspfHello>();
    uint32_t neighborIpInt = ospfHelloChunk->getSenderIp();
    auto ifaceInd = pkt->findTag<InterfaceInd>();
    NetworkInterface *arrivalIf = ift_->getInterfaceById(ifaceInd->getInterfaceId());
    
    // Build a Packet named OspfHello
    Packet *hello = new Packet("OspfHello");
    // Add a payload marker so it isn't empty (optional)
    auto ospfHelloFeedback = makeShared<OspfHello>();
    ospfHelloFeedback->setSenderIp(routerIdKey_);
    ospfHelloFeedback->setIsFeedback(true);
    hello->insertAtBack(ospfHelloFeedback);

    // Tag outgoing interface id (so ipOut knows which interface)
    hello->addTagIfAbsent<InterfaceReq>()->setInterfaceId(arrivalIf->getInterfaceId());

    // optionally add a small custom tag or param to mark as Hello
    // (we already used packet name "HELLO", which we check when receiving)
    EV_INFO << "MecOspf:sendHelloFeedback - sending OspfHello feedback from " << routerId_ << " (" << routerIdKey_ << ") to "
            << Ipv4Address(neighborIpInt)
                                << " via interface " << arrivalIf->getInterfaceName() << "\n";

    // send(hello, "ipOut");
    socket_.sendTo(hello, Ipv4Address::ALL_OSPF_ROUTERS_MCAST, MEC_OSPF_PORT);
}


/* === processHello ===
 * Receive a HELLO Packet; extract source IP (from L3AddressInd tag if present). 
 * Update neighbor entry and localLinks.
 */
void MecOspf::processHello(Packet *packet)
{
    // extract router ID from Hello chunk
    auto ospfHelloChunk = packet->peekAtFront<OspfHello>();
    if (!ospfHelloChunk->isFeedback()) {
        // this is the initial Hello from neighbor, send feedback Hello back
        EV_INFO << "MecOspf:processHello - received initial Hello from " << Ipv4Address(ospfHelloChunk->getSenderIp()) << ", sending feedback\n";
        sendHelloFeedback(packet);

        return;
    }

    // extract router ID of neighbor from Hello chunk
    Ipv4Address neighborIp = Ipv4Address::UNSPECIFIED_ADDRESS;
    if (ospfHelloChunk)
        neighborIp = Ipv4Address(ospfHelloChunk->getSenderIp());
    
    if (neighborIp.isUnspecified()) {
        std::cerr << NOW << " MecOspf::processHello - Hello chunk missing routerId\n";
        return;
    }
    
    NetworkInterface *arrivalIf = ift_->getInterfaceById(packet->findTag<InterfaceInd>()->getInterfaceId());
    EV_INFO << "MecOspf:processHello - received Hello feedback from " << neighborIp << " (" << ospfHelloChunk->getSenderIp() << ")"
            << " via interface " << arrivalIf->getInterfaceName() << "\n";

    // Attempt to get source address from L3AddressInd tag
    auto l3ind = packet->findTag<L3AddressInd>();
    Ipv4Address gateway = Ipv4Address::UNSPECIFIED_ADDRESS;
    if (l3ind) {
        // gateway is the ip address of the neighbor interface that our Hello arrived on
        gateway = l3ind->getSrcAddress().toIpv4();
    }
    // If we didn't find src in tags, try to derive from arrival gate's interface
    if (gateway.isUnspecified()) {
        // some setups put source info in packet tags; if not, we can only use interface
        // We'll log and return — Hello without source is unusable
        std::cerr << NOW << " MecOspf::processHello - no L3 tag found; cannot extract src IP (iface="
                << arrivalIf->getInterfaceName() << ")\n";
        return;
    }

    // Insert or refresh neighbor entry
    uint32_t key = ipKey(neighborIp);
    auto it = neighbors_.find(key);
    if (it == neighbors_.end()) {
        // new neighbor
        Neighbor n = Neighbor(neighborIp, gateway, arrivalIf, simTime(), 1.0);
        neighbors_.emplace(key, n);
        EV_INFO << "MecOspf:processHello - discovered neighbor " << neighborIp << " (discovered neighbors in total:" << neighbors_.size() << ")\n";

        // add route to neighbor
        if (rt_) {
            Ipv4Route *route = new Ipv4Route();
            route->setDestination(neighborIp);
            route->setNetmask(Ipv4Address::ALLONES_ADDRESS); // host route
            route->setGateway(gateway);
            route->setInterface(arrivalIf);
            route->setSourceType(Ipv4Route::OSPF);
            route->setMetric(1);
            rt_->addRoute(route);
            neighborRoutes_[key] = route;
            EV_INFO << "MecOspf:processHello - added direct route to neighbor " << neighborIp << "\n";
        }

        neighborChanged_ = true; // mark neighbor change happened
        newNeighbors_.push_back(key);
        resetGlobalScheduler(); // reset global scheduler info
        clearIndirectRoutes(); // clear cached indirect routes

        // Update adjacency map (bidirectional link with cost n.cost)
        topology_[routerIdKey_][key] = n.cost;
        EV_DETAIL << "MecOspf:processHello - updated LSA for " << routerId_ << ": seqNum=" << lsaPacketCache_[routerIdKey_]->getSeqNum()
                  << " neighbors=" << topology_[routerIdKey_].size() << "\n";
    }
    else {
        // refresh existing neighbor timestamp and interface
        it->second.lastSeen = simTime();
        EV_DETAIL << "MecOspf:processHello - refreshed neighbor " << neighborIp << "\n";
    }
}


/***
 * === sendLsa ===
 * Create and send a LSA Packet to all neighbors (multicast).
 * The LSA contains our router ID, sequence number, and list of neighbors.
 * Neighbors receiving the LSA can update their topology graph and recompute routes.
 */
void MecOspf::updateLsaToNetwork()
{
    if (!ift_) {
        EV_WARN << "MecOspf::updateLsaToNetwork - no InterfaceTable available\n";
        return;
    }

    // send our own updated LSA to all neighbors
    for (auto &kv : neighbors_)
    {
        Neighbor &n = kv.second;
        if (!n.outInterface || !n.outInterface->isUp() || n.outInterface->isWireless()) continue;

        EV_INFO << "MecOspf:sendLsaToNeighbor - sending LSA to neighbor " << n.destIp << "\n";
        sendLsa(selfLsa_, kv.first);
    }

    // send other LSAs that we know (in the lsaPacketCache_) to new neighbors 
    // (to help them build topology which they missed when they were down)
    if (!newNeighbors_.empty()) {
        for (auto key : newNeighbors_) {
            // make sure key (the int version of Ipv4Address) is valid
            auto it = neighbors_.find(key);
            if (it == neighbors_.end()) continue; // sanity check

            // make sure the neighbor interface is still up
            Neighbor &n = it->second;
            if (!n.outInterface || !n.outInterface->isUp() || n.outInterface->isWireless()) continue;

            EV_INFO << "MecOspf:sendLsaToNeighbor - new neighbor " << n.destIp << " discovered, sending all cached LSAs\n";
            // send all cached LSAs to this new neighbor, except our own (already sent above)
            for (const auto& lsaEntry : lsaPacketCache_) {
                uint32_t originKey = lsaEntry.first;
                if (originKey == routerIdKey_) continue; // skip our own LSA (already sent above)

                EV_INFO << "MecOspf:sendLsaToNeighbor - sending cached LSA originating from " << lsaEntry.second->getOrigin()
                        << " to new neighbor " << n.destIp << "\n";
                sendLsa(lsaEntry.second, key);
            }
        }
    }
}


/***
 * === handleLsa ===
 * check if the LSA is new (higher seqNum) and update lsdb_.
 * If updated, recompute routing and forward LSA to neighbors (except the one it came from).
 * If not new, discard.
 */
void MecOspf::handleReceivedLsa(Packet *packet)
{
    // Extract LSA from packet
    auto lsa = packet->peekAtFront<OspfLsa>();
    if (!lsa) {
        EV_WARN << "MecOspf:handleReceivedLsa - not an OSPF LSA packet\n";
        return;
    }

    // Check if LSA is new (higher seqNum)
    bool needUpdate = false;
    // if the origin router is not in cache, add it
    uint32_t originKey = lsa->getOrigin();
    if (lsaPacketCache_.find(originKey) == lsaPacketCache_.end())   // first time seeing this origin
    {
        needUpdate = true;
        ipv4ToMacNodeId_[originKey] = lsa->getNodeId();
    }    

    int lsaSeqNum = lsa->getSeqNum();
    if (!needUpdate && (lsaSeqNum > lsaPacketCache_[originKey]->getSeqNum()))   // not first time but higher seqNum
        needUpdate = true;

    if (needUpdate) {
        // Update LSA database
        lsaPacketCache_[originKey] = makeShared<OspfLsa>(*lsa);
        EV_INFO << "MecOspf:handleReceivedLsa - received updated LSA from " << Ipv4Address(originKey) << "\n";

        resetGlobalScheduler(); // reset global scheduler info
        updateTopologyFromLsa(lsa); // Update topology graph
        clearIndirectRoutes(); // clear cached indirect routes

        // get the arrival interface (to avoid sending back)
        auto ifaceInd = packet->findTag<InterfaceInd>();
        int arrivalIfId = ifaceInd->getInterfaceId();

        // Forward LSA to neighbors (except the one it came from)
        for (const auto& n : neighbors_) {
            if (n.second.outInterface->getInterfaceId() != arrivalIfId) {
                sendLsa(lsa, n.first);
            }
        }

        // schedule route recomputation after a short delay to ensure the propagation is complete
        simtime_t lsaInstallTime = lsa->getInstallTime();
        if (!routeComputationTimer_->isScheduled())
        {
            scheduleAt(lsaInstallTime + routeComputationDelay_, routeComputationTimer_);
            largestLsaTime_ = lsaInstallTime;

            EV << "MecOspf:handleReceivedLsa - scheduled route recomputation at " << (lsaInstallTime + routeComputationDelay_) << "\n";
        }
        else if (routeComputationTimer_->isScheduled() && largestLsaTime_ < lsaInstallTime)
        {
            // if some LSA is generated at a later time, we can cancel and reschedule
            // the routeComputationTimer_ to a later time to avoid multiple recomputations
            largestLsaTime_ = lsaInstallTime;
            cancelEvent(routeComputationTimer_);
            scheduleAt(largestLsaTime_ + routeComputationDelay_, routeComputationTimer_);

            EV << "MecOspf:handleReceivedLsa - rescheduled route recomputation at " << (largestLsaTime_ + routeComputationDelay_) << "\n";
        }
    } else {
        EV_INFO << "MecOspf:handleReceivedLsa - received old LSA from " << Ipv4Address(originKey) << ", ignore it!\n";
    }
}


/***
 * === updateTopologyFromLsa ===
 * Update our topology graph from a received LSA.
 */
void MecOspf::updateTopologyFromLsa(inet::Ptr<const OspfLsa>& lsa)
{
    if (!lsa) return;
    uint32_t originKey = lsa->getOrigin();

    int neighborCount = lsa->getNeighborArraySize();
    topology_[originKey].clear();
    for (int i = 0; i < neighborCount; i++) {
        uint32_t nbrKey = lsa->getNeighbor(i);
        double cost = lsa->getCost(i);
        topology_[originKey][nbrKey] = cost;
    }
    EV_DETAIL << "MecOspf:updateTopologyFromLsa - updated topology for " << Ipv4Address(originKey) << ":\n";
    for (const auto& kv : topology_[originKey]) {
        Ipv4Address nbrIp = Ipv4Address(kv.first);
        double cost = kv.second;
        EV_DETAIL << "\t" << nbrIp << " (cost=" << cost << ")\n";
    }
}


/***
 * === sendLsa ===
 */
void MecOspf::sendLsa(inet::Ptr<const OspfLsa> lsa, uint32_t neighborKey)
{
    Neighbor &n = neighbors_[neighborKey];

    // Build a Packet named OspfLsa
    Packet *lsaPkt = new Packet("OspfLsa");
    // Add a payload marker so it isn't empty (optional)
    auto lsaChunk = makeShared<OspfLsa>(*lsa);
    lsaPkt->insertAtBack(lsaChunk);
    lsaPkt->addTagIfAbsent<InterfaceReq>()->setInterfaceId(n.outInterface->getInterfaceId());    

    EV_INFO << "MecOspf:sendLsa - sending LSA (origin=" << lsa->getOrigin() << ", seqNum=" << lsa->getSeqNum()
            << ") to neighbor " << n.destIp << " via interface " << n.outInterface->getInterfaceName() << "\n";

    // send(lsaPkt, "ipOut");
    socket_.sendTo(lsaPkt, Ipv4Address::ALL_OSPF_ROUTERS_MCAST, MEC_OSPF_PORT);
}


/* === checkNeighborTimeouts ===
 * Remove neighbors that haven't been heard from within neighborTimeout.
 */
void MecOspf::checkNeighborTimeouts()
{
    EV << "MecOspf:checkNeighborTimeouts - checking neighbor timeouts\n";

    vector<uint32_t> toRemove;
    for (auto &kv : neighbors_) {
        const Neighbor &n = kv.second;
        if (simTime() - n.lastSeen >= neighborTimeout_) {
            toRemove.push_back(kv.first);
        }
    }
    if (toRemove.empty()) return;

    for (auto key : toRemove) {
        Ipv4Address neighborIp = neighbors_[key].destIp;
        EV_WARN << "MecOspf:checkNeighborTimeouts - neighbor " << neighborIp << " timed out -> remove\n";
        neighbors_.erase(key);
        Ipv4Route *route = rt_->removeRoute(neighborRoutes_[key]);
        if (route != nullptr)
            delete route;
        
        neighborRoutes_.erase(key);
    }

    neighborChanged_ = true; // mark neighbor change happened
    resetGlobalScheduler(); // reset global scheduler

    // start building new topology
    clearIndirectRoutes();
    for (auto key : toRemove) {
        topology_[routerIdKey_].erase(key);
    }
}

/* === clearInstalledRoutes ===
 * Remove routes we previously installed (to avoid accumulating stale routes).
 */
void MecOspf::clearIndirectRoutes()
{
    if (!rt_ || indirectRoutes_.empty()) return;

    EV_INFO << "MecOspf::clearIndirectRoutes - removing " << indirectRoutes_.size() << " routes\n";
    for (auto r : indirectRoutes_) {
        Ipv4Route *route = rt_->removeRoute(r.second);
        if (route != nullptr)
            delete route;
    }

    indirectRoutes_.clear();
}

void MecOspf::clearNeighborRoutes()
{
    if (!rt_ || neighborRoutes_.empty()) return;
 
    EV_INFO << "MecOspf::clearNeighborRoutes - removing " << neighborRoutes_.size() << " routes\n";
    for (auto r : neighborRoutes_) {
        Ipv4Route *route = rt_->removeRoute(r.second);
        if (route != nullptr)
            delete route;
    }
    neighborRoutes_.clear();
}

/* === recomputeRouting ===
 * Run Dijkstra to compute shortest paths from ourself to all nodes in the topology.
 * Install indirect routes in the routing table accordingly (neighbor routes do not need to change).
 */
void MecOspf::recomputeIndirectRouting()
{
    if (!ift_ || !rt_) {
        EV_WARN << "MecOspf::recomputeIndirectRouting - missing ift_ or rt_\n";
        return;
    }

    EV_INFO << "MecOspf::recomputeIndirectRouting - Run Dijkstra to determine updated indirect routes\n";

    // ======== Step 1: clear previously installed indirect routes ========
    clearIndirectRoutes();

    // ======== Step 2: Dijkstra's algorithm to find shortest paths from routerIdKey_ to all other nodes ========
    // initialize node infos
    std::map<uint32_t, Node> nodeInfos;
    for (const auto& kv : topology_) {
        uint32_t nodeKey = kv.first;
        nodeInfos[nodeKey] = {nodeKey, INFINITY, 0, false};
        if (nodeKey == routerIdKey_) {
            nodeInfos[nodeKey].cost = 0.0; // cost to self is 0
        }
    }

    dijkstra(routerIdKey_, nodeInfos);

    // ======== Step 3: extract next-hop routing table from Dijkstra results ========
    vector<uint32_t> reachableNodes = {routerIdKey_};    // in case the topology is partitioned
    // For each destination, we backtrack the predecessor chain until we reach the direct neighbor of routerIdKey_.
    for (const auto& kv : nodeInfos) {
        uint32_t dest = kv.first;
        if (dest == routerIdKey_) continue;  // skip self

        // Unreachable?
        if (kv.second.cost == INFINITY || kv.second.prevHop == 0) {
            EV_INFO << "Destination " << Ipv4Address(dest) << " is unreachable\n";
            topology_.erase(dest); // remove unreachable node from topology
            lsaPacketCache_.erase(dest); // remove its LSA from cache
            continue;
        }

        reachableNodes.push_back(dest);
        // Backtrack from dest to source
        uint32_t current = dest;
        uint32_t prev = nodeInfos[current].prevHop;

        // skip if direct neighbor
        if (prev == routerIdKey_) continue;

        // Walk backwards until the predecessor is the source
        EV_INFO << "Path to " << Ipv4Address(dest) << ": " << Ipv4Address(current);
        while (prev != 0 && prev != routerIdKey_) {
            current = prev;
            prev = nodeInfos[current].prevHop;
            EV_INFO << " <- " << Ipv4Address(current);
        }
        EV_INFO << " <- " << Ipv4Address(prev);
        EV_INFO << " (cost=" << kv.second.cost << ")\n";

        // Add the next hop route into the routing table
        // current is now the first hop on the path from routerIdKey_ to dest
        if (prev == routerIdKey_) {
            // add routes to the routingTable
            auto it = neighbors_.find(current);
            if (it == neighbors_.end()) {
                EV_WARN << "MecOspf:recomputeIndirectRouting - next hop neighbor " << Ipv4Address(current) << " not found in neighbors, skip route to "
                        << Ipv4Address(dest) << "\n";
                continue;
            }
            Neighbor &n = it->second;
            if (rt_) {
                Ipv4Route *route = new Ipv4Route();
                route->setDestination(Ipv4Address(dest));
                route->setNetmask(Ipv4Address::ALLONES_ADDRESS); // host route
                route->setGateway(n.gateway);
                route->setInterface(n.outInterface);
                route->setSourceType(Ipv4Route::OSPF);
                route->setMetric(1);
                rt_->addRoute(route);
                indirectRoutes_[dest] = route;
                EV_INFO << "MecOspf:processHello - added indirect route to node " << Ipv4Address(dest) << "\n";
            }
        }
    }
    // if (nodeInfo_) {
    //     nodeInfo_->setRtState(true); // mark routing table is ready
    // }

    // ======= Step 4: determine the scheduler node ========
    // select the reachable node with maximum number of neighbors as the scheduler
    // (if multiple, select the one with the lowest IP address)
    // because 
    size_t maxNeighbors = 0;
    for (auto nodeKey : reachableNodes) {
        // only consider nodes has positive nodeId (i.e., gNBs)
        if (lsaPacketCache_[nodeKey]->getNodeId() <= 0) continue;

        size_t nbrCount = topology_[nodeKey].size();
        if (nbrCount > maxNeighbors) {
            maxNeighbors = nbrCount;
            schedulerAddr_ = Ipv4Address(nodeKey);
        } else if (nbrCount == maxNeighbors) {
            // If tie, select the one with the lowest IP address
            Ipv4Address candidate = Ipv4Address(nodeKey);
            if (candidate < schedulerAddr_) {
                schedulerAddr_ = candidate;
            }
        }
    }

    if (schedulerAddr_ == routerId_)
    {
        EV_INFO << "MecOspf:recomputeIndirectRouting - this node is selected as the scheduler node (neighbors=" << maxNeighbors << ")\n";
    }
    else
    {
        int globalSchedulerNodeId = lsaPacketCache_[schedulerAddr_.getInt()]->getNodeId();
        EV_INFO << "MecOspf:recomputeIndirectRouting - selected gNB node " << globalSchedulerNodeId
        << " (IP address: " << schedulerAddr_ << ", neighbors=" << maxNeighbors << ") as the global scheduler.\n";
    }

    globalSchedulerReady_ = true;
    if (nodeInfo_)
    {
        nodeInfo_->setGlobalSchedulerAddr(schedulerAddr_);
        updateAdjListToScheduler();
    }
}


void MecOspf::dijkstra(uint32_t source, std::map<uint32_t, Node>& nodeInfos)
{
    // Initialize priority queue
    auto cmp = [](const Node* a, const Node* b) { return a->cost > b->cost; };
    std::priority_queue<Node*, std::vector<Node*>, decltype(cmp)> pq(cmp);

    // Start from source
    pq.push(&nodeInfos[source]);
    while (!pq.empty()) {
        Node* current = pq.top();
        pq.pop();
        if (current->visited) continue;     // skip stale entries
        current->visited = true;

        // Explore neighbors
        for (const auto& neighborKv : topology_[current->nodeKey]) {
            uint32_t neighborKey = neighborKv.first;
            double linkCost = neighborKv.second;
            if (nodeInfos.find(neighborKey) == nodeInfos.end()) continue; // unknown node

            Node &neighborInfo = nodeInfos[neighborKey];
            if (current->cost + linkCost < neighborInfo.cost) {
                neighborInfo.cost = current->cost + linkCost;
                neighborInfo.prevHop = current->nodeKey;
                pq.push(&neighborInfo);
            }
        }
    }
}


void MecOspf::updateAdjListToScheduler()
{
    if (globalSchedulerReady_ && nodeInfo_ && nodeInfo_->getIsGlobalScheduler())
    {
        EV << "MecOspf:updateAdjListToScheduler - updating adjacency list (network topology) to scheduler\n";
        map<MacNodeId, map<MacNodeId, double>> adjList;
        for (const auto& kv : topology_)    // map<uint32_t, map<uint32_t, double>> topology_
        {
            MacNodeId src = ipv4ToMacNodeId_[kv.first];
            for (const auto& nbr : kv.second)
            {
                MacNodeId dst = ipv4ToMacNodeId_[nbr.first];
                double cost = nbr.second;
                adjList[src][dst] = cost;
            }
        }
        nodeInfo_->updateAdjListToScheduler(adjList);
    }
}


void MecOspf::resetGlobalScheduler()
{
    if (globalSchedulerReady_)
    {
        EV_INFO << "MecOspf:resetGlobalScheduler - resetting global scheduler\n";

        if (nodeInfo_)
            nodeInfo_->setGlobalSchedulerAddr(Ipv4Address::UNSPECIFIED_ADDRESS);
        
        globalSchedulerReady_ = false;
        schedulerAddr_ = Ipv4Address::UNSPECIFIED_ADDRESS;
    }
}


void MecOspf::handleNodeFailure()
{
    Enter_Method("handleNodeFailure");

    EV_INFO << "MecOspf:handleNodeFailure - handling node failure, cleaning up state\n";
    // cancel timers
    if (lsaTimer_->isScheduled())
        cancelEvent(lsaTimer_);
    if (routeComputationTimer_->isScheduled())
        cancelEvent(routeComputationTimer_);
    
    // clear all routes
    clearIndirectRoutes();
    clearNeighborRoutes();

    // clear neighbor information
    neighbors_.clear();
    neighborChanged_ = true;
    topology_[routerIdKey_].clear();

    globalSchedulerReady_ = false;
    schedulerAddr_ = Ipv4Address::UNSPECIFIED_ADDRESS;
}


// Helper: get local IP associated with a gate (if needed)
Ipv4Address MecOspf::getLocalAddressOnGate(cGate *gate)
{
    if (!ift_ || !gate) return Ipv4Address::UNSPECIFIED_ADDRESS;
    NetworkInterface *ie = ift_->findInterfaceByNodeOutputGateId(gate->getId());
    if (!ie) return Ipv4Address::UNSPECIFIED_ADDRESS;
    return ie->getIpv4Address();
}

// cleanup
void MecOspf::finish()
{
    clearIndirectRoutes();
    clearNeighborRoutes();
}
