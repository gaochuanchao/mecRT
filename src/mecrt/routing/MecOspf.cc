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
    helloTimer = nullptr;
    helloFeedbackTimer = nullptr;
    ift = nullptr;
    rt = nullptr;
}

MecOspf::~MecOspf()
{
    if (helloTimer) {
        cancelAndDelete(helloTimer);
        helloTimer = nullptr;
    }
    clearIndirectRoutes();
    clearNeighborRoutes();
}


void MecOspf::initialize(int stage)
{
    RoutingProtocolBase::initialize(stage);

    if (stage == INITSTAGE_LOCAL) {
        // create timers
        helloTimer = new cMessage("helloTimer");
        lsaTimer = new cMessage("lsaTimer");

        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
    }
    else if (isInitializeStage(stage)) {
        if (enableInitDebug_)
            EV_INFO << "MecOspf:initialize - initialize stage " << stage << "\n";

        EV_INFO << "MecOspf:initialize - network-layer init at stage " << stage << "\n";
        // acquire required INET modules via parameters (StandardHost uses these params)
        try {
            ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);

            // make sure our module joins the OSPF multicast groups on all interfaces
            // (otherwise we won't receive OSPF multicast packets)
            for (int i = 0; i < ift->getNumInterfaces(); i++) {
                NetworkInterface *ie = ift->getInterface(i);
                if (ie->isLoopback()) continue; // skip loopback interface
                ie->joinMulticastGroup(Ipv4Address::ALL_OSPF_ROUTERS_MCAST);
                ie->joinMulticastGroup(Ipv4Address::ALL_OSPF_DESIGNATED_ROUTERS_MCAST);
            }

            EV_INFO << "MecOspf:initialize - interfaceTableModule found\n";
        } catch (cException &e) {
            cerr << "MecOspf: cannot find interfaceTableModule param\n";
            ift = nullptr;
        }
        try {
            rt = getModuleFromPar<Ipv4RoutingTable>(par("routingTableModule"), this);
            routerId_ = rt->getRouterId();
            routerIdKey_ = routerId_.getInt();

            EV_INFO << "MecOspf:initialize - routingTableModule found, routerId=" << routerId_ << "\n";
        } catch (cException &e) {
            cerr << "MecOspf:initialize - cannot find routingTableModule param\n";
            rt = nullptr;
        }

        // register protocol such that the IP layer can deliver packets to us
        registerProtocol(MecProtocol::mecOspf, gate("ipOut"), gate("ipIn"));

        EV_INFO << "MecOspf:initialize - network-layer init. Interfaces=" << (ift ? ift->getNumInterfaces() : 0)
                << " RoutingTable=" << (rt ? "found" : "NOT_FOUND") << "\n";

        // initialize its Link State Advertisement (LSA) info
        seqNum_ = 0;
        LsaInfo lsa(routerId_, seqNum_, simTime());
        lsdb[routerId_] = lsa;

        WATCH(routerId_);
        WATCH(routerIdKey_);
        WATCH_MAP(indirectRoutes);
        WATCH_MAP(neighborRoutes);
    }
}

// central message dispatcher
void MecOspf::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // timer fired
        EV_INFO << "MecOspf:handleMessageWhenUp - self-message " << msg->getName() << "\n";
        handleSelfTimer(msg);
        return;
    }
    else
    {
        Packet *packet = check_and_cast<Packet *>(msg);
        const char *pname = packet->getName();

        if (pname && strcmp(pname, "OspfHello") == 0) {
            EV_INFO << "MecOspf:handleMessageWhenUp - OspfHello received\n";
            processHello(packet);
            return;
        }
        else if (pname && strcmp(pname, "OspfLsa") == 0) {
            EV_INFO << "MecOspf:handleMessageWhenUp - OspfLsa received\n";
            forwardLsa(packet);
            return;
        }
        else
        {
            // Otherwise treat as data packet to forward
            EV_WARN << "MecOspf:handleMessageWhenUp - data packet received: " << (pname ? pname : "(unnamed)") << ", delete!\n";
            delete msg;
            return;
        }
    }
}


/* === handleTimer ===
 * Called when helloTimer fires. Send Hello, check neighbor timeouts, reschedule.
 */
void MecOspf::handleSelfTimer(cMessage *msg)
{
    if (msg == helloTimer)
    {
        EV_INFO << "MecOspf::handleSelfTimer - Hello timer fired at " << simTime() << "\n";

        // send Hellos
        sendInitialHello();
        // detect neighbor timeouts (link failure)
        checkNeighborTimeouts();
        // reschedule next Hello, continuous monitor neighbor accessibility
        scheduleAt(simTime() + helloInterval, helloTimer);
        // also schedule LSA synchronization timer
        scheduleAt(simTime() + lsaWaitInterval, lsaTimer);
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
    if (!ift) {
        EV_WARN << "MecOspf::sendInitialHello - no IInterfaceTable available\n";
        return;
    }

    // iterate all interfaces and send Hello where appropriate
    for (int i = 0; i < ift->getNumInterfaces(); ++i) {
        NetworkInterface *ie = ift->getInterface(i);
        if (!ie || ie->isLoopback() || !ie->isUp() || ie->isWireless()) // skip loopback/down/wireless interfaces
            continue;

        // get the IPv4 address for this interface
        Ipv4Address local = ie->getIpv4Address();
        if (local.isUnspecified()) continue;

        // Build a Packet named OspfHello
        Packet *hello = new Packet("OspfHello");
        // Add a payload marker so it isn't empty (optional)
        auto ospfHelloChunk = makeShared<OspfHello>();
        ospfHelloChunk->setSenderIp(routerIdKey_);
        hello->insertAtBack(ospfHelloChunk);

        // add dispatch tags and protocols
        hello->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
        hello->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&MecProtocol::mecOspf);
        // Tag destination address (multicast all-hosts)
        auto addrReq = hello->addTagIfAbsent<L3AddressReq>();
        addrReq->setDestAddress(Ipv4Address::ALL_OSPF_ROUTERS_MCAST);
        addrReq->setSrcAddress(local);  // used as gateway for neighbor to reach us
        // Tag outgoing interface id (so ipOut knows which interface)
        hello->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
        // Hop limit (Hello should not be forwarded across multiple hops)
        hello->addTagIfAbsent<HopLimitReq>()->setHopLimit(1);

        // optionally add a small custom tag or param to mark as Hello
        // (we already used packet name "HELLO", which we check when receiving)
        EV_INFO << "MecOspf:sendInitialHello - sending OspfHello from " << routerId_ << " (" << routerIdKey_ << ")"
                                    << " via interface " << ie->getInterfaceName() << "\n";

        send(hello, "ipOut");
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
    NetworkInterface *arrivalIf = ift->getInterfaceById(ifaceInd->getInterfaceId());

    // Build a Packet named OspfHello
    Packet *hello = new Packet("OspfHello");
    // Add a payload marker so it isn't empty (optional)
    auto ospfHelloFeedback = makeShared<OspfHello>();
    ospfHelloFeedback->setSenderIp(routerIdKey_);
    ospfHelloFeedback->setNeighborIp(neighborIpInt);
    hello->insertAtBack(ospfHelloFeedback);

    // add dispatch tags
    hello->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
    // add the packet protocol tag as OSPF (we'll use IPv4 as the carrier)
    hello->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&MecProtocol::mecOspf);
    // Tag destination address (multicast all-hosts)
    auto addrReq = hello->addTagIfAbsent<L3AddressReq>();
    addrReq->setDestAddress(Ipv4Address::ALL_OSPF_ROUTERS_MCAST);
    addrReq->setSrcAddress(arrivalIf->getIpv4Address());    // used as gateway for neighbor to reach us    
    // Tag outgoing interface id (so ipOut knows which interface)
    hello->addTagIfAbsent<InterfaceReq>()->setInterfaceId(arrivalIf->getInterfaceId());
    // Hop limit (Hello should not be forwarded across multiple hops)
    hello->addTagIfAbsent<HopLimitReq>()->setHopLimit(1);

    // optionally add a small custom tag or param to mark as Hello
    // (we already used packet name "HELLO", which we check when receiving)
    EV_INFO << "MecOspf:sendHelloFeedback - sending OspfHello feedback from " << routerId_ << " (" << routerIdKey_ << ") to "
            << Ipv4Address(neighborIpInt)
                                << " via interface " << arrivalIf->getInterfaceName() << "\n";

    send(hello, "ipOut");
}


/* === processHello ===
 * Receive a HELLO Packet; extract source IP (from L3AddressInd tag if present). 
 * Update neighbor entry and localLinks.
 */
void MecOspf::processHello(Packet *packet)
{
    // extract router ID from Hello chunk
    auto ospfHelloChunk = packet->peekAtFront<OspfHello>();
    if (ospfHelloChunk->getNeighborIp() != routerIdKey_) {
        // this is the initial Hello from neighbor, send feedback Hello back
        EV_INFO << "MecOspf:processHello - received initial Hello from " << ospfHelloChunk->getSenderIp() << ", sending feedback\n";
        sendHelloFeedback(packet);

        // free packet
        delete packet;
        return;
    }

    // extract router ID of neighbor from Hello chunk
    Ipv4Address neighborIp = Ipv4Address::UNSPECIFIED_ADDRESS;
    if (ospfHelloChunk)
        neighborIp = Ipv4Address(ospfHelloChunk->getSenderIp());
    
    if (neighborIp.isUnspecified()) {
        std::cerr << NOW << " MecOspf::processHello - Hello chunk missing routerId\n";
        delete packet;
        return;
    }
    
    NetworkInterface *arrivalIf = ift->getInterfaceById(packet->findTag<InterfaceInd>()->getInterfaceId());
    if (!arrivalIf) {
        std::cerr << NOW << " MecOspf:processHello - received packet on unknown interface\n";
        delete packet;
        return; 
    }

    EV_INFO << "MecOspf:processHello - received Hello feedback from " << neighborIp << " (" << ospfHelloChunk->getSenderIp() << ")"
            << " via interface " << arrivalIf->getInterfaceName() << "\n";

    // Attempt to get source address from L3AddressInd tag
    auto l3ind = packet->findTag<L3AddressInd>();
    Ipv4Address gateway = Ipv4Address::UNSPECIFIED_ADDRESS;
    if (l3ind) {
        // L3AddressInd provides canonical L3 info
        gateway = l3ind->getSrcAddress().toIpv4();
    }
    // If we didn't find src in tags, try to derive from arrival gate's interface
    if (gateway.isUnspecified()) {
        // some setups put source info in packet tags; if not, we can only use interface
        // We'll log and return — Hello without source is unusable
        std::cerr << NOW << " MecOspf::processHello - no L3 tag found; cannot extract src IP (iface="
                << arrivalIf->getInterfaceName() << ")\n";
        delete packet;
        return;
    }

    // Insert or refresh neighbor entry
    uint32_t key = ipKey(neighborIp);
    auto it = neighbors.find(key);
    if (it == neighbors.end()) {
        // new neighbor
        Neighbor n = Neighbor(neighborIp, arrivalIf, simTime(), 1.0);
        neighbors.emplace(key, n);
        EV_INFO << "MecOspf:processHello - discovered neighbor " << neighborIp << " (discovered neighbors in total:" << neighbors.size() << ")\n";

        // add route to neighbor
        if (rt) {
            Ipv4Route *route = new Ipv4Route();
            route->setDestination(neighborIp);
            route->setNetmask(Ipv4Address::ALLONES_ADDRESS); // host route
            route->setGateway(gateway);
            route->setInterface(arrivalIf);
            route->setSourceType(Ipv4Route::OSPF);
            route->setMetric(1);
            rt->addRoute(route);
            neighborRoutes[key] = route;
            EV_INFO << "MecOspf:processHello - added direct route to neighbor " << neighborIp << "\n";
        }

        // Update adjacency map (bidirectional link with cost n.cost)
        lsdb[routerId_].neighbors[neighborIp] = n.cost;
        lsdb[routerId_].installTime = simTime(); // update our LSA install time
        lsdb[routerId_].seqNum++; // increment our LSA sequence number
        EV_DETAIL << "MecOspf:processHello - updated LSA for " << routerId_ << ": seqNum=" << lsdb[routerId_].seqNum
                  << " installTime=" << lsdb[routerId_].installTime
                  << " neighbors=" << lsdb[routerId_].neighbors.size() << "\n";
    }
    else {
        // refresh existing neighbor timestamp and interface
        it->second.lastSeen = simTime();
        lsdb[routerId_].installTime = simTime(); // update our LSA install time
        EV_DETAIL << "MecOspf:processHello - refreshed neighbor " << neighborIp << "\n";
    }

    // free packet
    delete packet;
}

/* === forwardPacket ===
 * Use L3 tag to get destination; find best route from Ipv4RoutingTable.
 * On forwarding: set InterfaceReq tag so ipOut sends via the chosen interface.
 */
void MecOspf::forwardLsa(Packet *packet)
{
    // Find destination info (L3AddressInd tag for received packets)
    auto l3ind = packet->findTag<L3AddressInd>();
    if (!l3ind) {
        EV_WARN << "MecOspf::forwardPacket - missing L3AddressInd tag; dropping\n";
        delete packet;
        return;
    }

    L3Address destAddr = l3ind->getDestAddress();
    if (destAddr.getType() != L3Address::IPv4) {
        EV_WARN << "MecOspf::forwardPacket - non-IPv4 dest; dropping\n";
        delete packet;
        return;
    }
    Ipv4Address dest = destAddr.toIpv4();

    if (!rt) {
        EV_WARN << "MecOspf::forwardPacket - no routing table available; dropping\n";
        delete packet;
        return;
    }

    // find best matching route
    const Ipv4Route *route = rt->findBestMatchingRoute(dest);
    if (!route) {
        EV_WARN << "MecOspf::forwardPacket - no route to " << dest << "; dropping\n";
        delete packet;
        return;
    }

    // prepare outgoing packet tags:
    // - set InterfaceReq to route's interface id
    // - set L3AddressReq dest to route->getGateway() or dest if gateway unspecified
    packet->addTagIfAbsent<InterfaceReq>()->setInterfaceId(route->getInterface()->getInterfaceId());

    Ipv4Address nextHop = route->getGateway();
    if (nextHop.isUnspecified()) nextHop = dest;

    packet->addTagIfAbsent<L3AddressReq>()->setDestAddress(nextHop);
    // send out via ipOut using interface's output gate id
    int outGateId = route->getInterface()->getNodeOutputGateId();
    EV_INFO << "MecOspf::forwardPacket - forwarding to " << dest
                                << " via nextHop " << nextHop << " outIf=" << route->getInterface()->getInterfaceName() << "\n";
    send(packet, "ipOut", outGateId);
}


/* === checkNeighborTimeouts ===
 * Remove neighbors that haven't been heard from within neighborTimeout.
 */
void MecOspf::checkNeighborTimeouts()
{
    vector<uint32_t> toRemove;
    for (auto &kv : neighbors) {
        const Neighbor &n = kv.second;
        if (simTime() - n.lastSeen > neighborTimeout) {
            toRemove.push_back(kv.first);
        }
    }
    if (toRemove.empty()) return;

    for (auto key : toRemove) {
        Ipv4Address neighborIp = neighbors[key].destIp;
        EV_WARN << "MecOspf:checkNeighborTimeouts - neighbor " << neighborIp << " timed out -> remove\n";
        neighbors.erase(key);
        rt->removeRoute(neighborRoutes[key]);
        delete neighborRoutes[key];
        neighborRoutes.erase(key);
    }

    // start building new topology
    clearIndirectRoutes();
    lsdb[routerId_].installTime = simTime();
    lsdb[routerId_].seqNum++;
    for (auto key : toRemove) {
        Ipv4Address neighborIp = Ipv4Address(key);
        lsdb[routerId_].neighbors.erase(neighborIp);
    }

    // TODO: start propagating link-state updates to other neighbors to build new topology

}

/* === clearInstalledRoutes ===
 * Remove routes we previously installed (to avoid accumulating stale routes).
 */
void MecOspf::clearIndirectRoutes()
{
    if (!rt) return;
    if (indirectRoutes.empty()) {
        EV_INFO << "MecOspf::clearIndirectRoutes - none installed\n";
        return;
    }
    EV_INFO << "MecOspf::clearIndirectRoutes - removing " << indirectRoutes.size() << " routes\n";
    for (auto r : indirectRoutes) {
        rt->removeRoute(r.second);
        delete r.second;
    }
    indirectRoutes.clear();
}

void MecOspf::clearNeighborRoutes()
{
    if (!rt) return;
    if (neighborRoutes.empty()) {
        EV_INFO << "MecOspf::clearNeighborRoutes - none installed\n";
        return;
    }
    EV_INFO << "MecOspf::clearNeighborRoutes - removing " << neighborRoutes.size() << " routes\n";
    for (auto r : neighborRoutes) {
        rt->removeRoute(r.second);
        delete r.second;
    }
    neighborRoutes.clear();
}

/* === recomputeRouting ===
 * Build a graph from localLinks and neighbors (optionally LSAs if you extend)
 * Run Dijkstra from one of our local addresses and install host (/32) routes.
 */
void MecOspf::recomputeIndirectRouting()
{
    if (!ift || !rt) {
        EV_WARN << "MecOspf::recomputeIndirectRouting - missing ift or rt\n";
        return;
    }

    EV_INFO << "MecOspf::recomputeIndirectRouting - starting SPF computation\n";

    // 1) Build node list and node index map
    vector<Ipv4Address> nodes;
    unordered_map<uint32_t,int> idx;   // ipKey -> index in nodes, assisting node lookup
    
    // an assistant function to add a node if not already present and record its index in the vector
    auto addNode = [&](const Ipv4Address &a){
        if (a.isUnspecified()) return;
        uint32_t k = ipKey(a);
        if (idx.count(k) == 0) { idx[k] = nodes.size(); nodes.push_back(a); }
    };

    // add our local IP addresses
    for (int i = 0; i < ift->getNumInterfaces(); ++i) {
        NetworkInterface *ie = ift->getInterface(i);
        if (!ie || ie->isLoopback()) continue;
        auto addr = ie->getIpv4Address();
        if (addr.isUnspecified()) continue;
        addNode(addr);
    }
}

// Helper: get local IP associated with a gate (if needed)
Ipv4Address MecOspf::getLocalAddressOnGate(cGate *gate)
{
    if (!ift || !gate) return Ipv4Address::UNSPECIFIED_ADDRESS;
    NetworkInterface *ie = ift->findInterfaceByNodeOutputGateId(gate->getId());
    if (!ie) return Ipv4Address::UNSPECIFIED_ADDRESS;
    return ie->getIpv4Address();
}


void MecOspf::handleStartOperation(LifecycleOperation *operation)
{
    if (enableInitDebug_)
        cout << "MecOspf::handleStartOperation - OSPF starting up\n";

    EV_INFO << "MecOspf::handleStartOperation - starting the OSPF protocol\n";
    clearIndirectRoutes();
    clearNeighborRoutes();

    simtime_t startupTime = par("startupTime");
    scheduleAfter(startupTime, helloTimer);
}

void MecOspf::handleStopOperation(LifecycleOperation *operation)
{
    EV_INFO << "MecOspf::handleStopOperation - stopping the OSPF protocol\n";
    cancelEvent(helloTimer);
    clearIndirectRoutes();
    clearNeighborRoutes();
    neighbors.clear();
    lsdb.clear();
}

void MecOspf::handleCrashOperation(LifecycleOperation *operation)
{
    EV_INFO << "MecOspf::handleCrashOperation - the OSPF protocol is crashed\n";
    cancelEvent(helloTimer);
    clearIndirectRoutes();
    clearNeighborRoutes();
    neighbors.clear();
    lsdb.clear();
}

// cleanup
void MecOspf::finish()
{
    if (helloTimer) {
        cancelAndDelete(helloTimer);
        helloTimer = nullptr;
    }
    clearIndirectRoutes();
    clearNeighborRoutes();
    EV_INFO << "MecOspf::finish - cleanup done\n";
}
