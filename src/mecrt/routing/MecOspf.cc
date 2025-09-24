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
#include "inet/routing/ospf_common/OspfPacketBase_m.h"
#include <queue>

Define_Module(MecOspf);


MecOspf::MecOspf()
{
    helloTimer = nullptr;
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
        // create timer
        helloTimer = new cMessage("helloTimer");

        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
    }
    else if (stage == INITSTAGE_ROUTING_PROTOCOLS) {
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
        } catch (cException &e) {
            EV_WARN << "MecOspf: cannot find interfaceTableModule param\n";
            ift = nullptr;
        }
        try {
            rt = getModuleFromPar<Ipv4RoutingTable>(par("routingTableModule"), this);
            routerId_ = rt->getRouterId();
            routerIdKey_ = routerId_.getInt();
        } catch (cException &e) {
            EV_WARN << "MecOspf: cannot find routingTableModule param\n";
            rt = nullptr;
        }

        // register protocol such that the IP layer can deliver packets to us
        registerProtocol(Protocol::ospf, gate("ipOut"), gate("ipIn"));

        EV_INFO << "MecOspf: network-layer init. Interfaces=" << (ift ? ift->getNumInterfaces() : 0)
                << " RoutingTable=" << (rt ? "found" : "NOT_FOUND") << "\n";
    }
}

// central message dispatcher
void MecOspf::handleMessageWhenUp(cMessage *msg)
{
    if (msg->isSelfMessage()) {
        // timer fired
        EV_INFO << "MecOspf: self-message " << msg->getName() << "\n";
        handleSelfTimer(msg);
        return;
    }
    else
    {
        Packet *packet = check_and_cast<Packet *>(msg);
        const char *pname = packet->getName();

        if (pname && strcmp(pname, "HELLO") == 0) {
            EV_INFO << "MecOspf: HELLO received\n";
            processHello(packet);
            return;
        }
        else
        {
            // Otherwise treat as data packet to forward
            EV_INFO << "MecOspf: data packet received: " << (pname ? pname : "(unnamed)") << "\n";
            forwardPacket(packet);
        }
    }
}


/* === handleTimer ===
 * Called when helloTimer fires. Send Hello, check neighbor timeouts, reschedule.
 */
void MecOspf::handleSelfTimer(cMessage *msg)
{
    if (msg != helloTimer) return;

    EV_INFO << "MecOspf::handleSelfTimer: Hello timer fired at " << simTime() << "\n";

    // send Hellos
    sendHello();

    // detect neighbor timeouts (link failure)
    checkNeighborTimeouts();

    // reschedule next Hello
    scheduleAt(simTime() + helloInterval, helloTimer);
}


/* === sendHello ===
 * Create a Packet named "HELLO" and tag it with destination address and outgoing interface.
 * - Every router sends Hello packets periodically out each interface.
 * - If a neighbor responds (or its Hello arrives), they become “neighbors.”
 * - If no Hello is seen for a Dead Interval → neighbor considered lost.
 */
void MecOspf::sendHello()
{
    if (!ift) {
        EV_WARN << "MecOspf::sendHello: no IInterfaceTable available\n";
        return;
    }

    // iterate all interfaces and send Hello where appropriate
    for (int i = 0; i < ift->getNumInterfaces(); ++i) {
        NetworkInterface *ie = ift->getInterface(i);
        if (!ie || ie->isLoopback() || !ie->isUp()) // skip loopback/down interfaces
            continue;

        // get the IPv4 address for this interface
        Ipv4Address local = ie->getIpv4Address();
        if (local.isUnspecified()) continue;

        // Build a Packet named HELLO
        Packet *hello = new Packet("HELLO");
        // Add a payload marker so it isn't empty (optional)
        auto ospfHelloChunk = makeShared<ospfv2::OspfPacketBase>();
        ospfHelloChunk->setRouterID(routerId_); // set the sender's IP address in the chunk
        ospfHelloChunk->setVersion(2);
        ospfHelloChunk->setType(ospfv2::HELLO_PACKET);
        hello->insertAtBack(ospfHelloChunk);

        // add the packet protocol tag as OSPF (we'll use IPv4 as the carrier)
        hello->addTagIfAbsent<PacketProtocolTag>()->setProtocol(&Protocol::ospf);
        // add dispatch tags
        hello->addTagIfAbsent<DispatchProtocolReq>()->setProtocol(&Protocol::ipv4);
        // Tag destination address (multicast all-hosts)
        auto addrReq = hello->addTagIfAbsent<L3AddressReq>();
        addrReq->setDestAddress(Ipv4Address::ALL_OSPF_ROUTERS_MCAST);
        addrReq->setSrcAddress(local);
        // Tag outgoing interface id (so ipOut knows which interface)
        hello->addTagIfAbsent<InterfaceReq>()->setInterfaceId(ie->getInterfaceId());
        // Hop limit (Hello should not be forwarded across multiple hops)
        hello->addTagIfAbsent<HopLimitReq>()->setHopLimit(1);

        // optionally add a small custom tag or param to mark as Hello
        // (we already used packet name "HELLO", which we check when receiving)
        EV_INFO << "MecOspf: sending HELLO from " << local
                                    << " via interface " << ie->getInterfaceName() << "\n";

        send(hello, "ipOut");
    }
}

/* === processHello ===
 * Receive a HELLO Packet; extract source IP (from L3AddressInd tag if present). 
 * Update neighbor entry and localLinks.
 */
void MecOspf::processHello(Packet *packet)
{
    NetworkInterface *arrivalIf = ift->getInterfaceById(packet->findTag<InterfaceInd>()->getInterfaceId());
    if (!arrivalIf) {
        EV_WARN << "MecOspf: received packet on unknown interface\n";
        delete packet;
        return;
    }

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
        EV_WARN << "MecOspf::processHello: no L3 tag found; cannot extract src IP (iface="
                << arrivalIf->getInterfaceName() << ")\n";
        delete packet;
        return;
    }

    // extract router ID from Hello chunk
    auto ospfHelloChunk = packet->peekAtFront<ospfv2::OspfPacketBase>();
    Ipv4Address neighborIp = ospfHelloChunk ? ospfHelloChunk->getRouterID() : Ipv4Address::UNSPECIFIED_ADDRESS;
    if (neighborIp.isUnspecified()) {
        EV_WARN << "MecOspf::processHello: Hello chunk missing routerId\n";
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
        EV_INFO << "MecOspf: discovered neighbor " << neighborIp << " (discovered neighbors in total:" << neighbors.size() << ")\n";

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
            EV_INFO << "MecOspf: added direct route to neighbor " << neighborIp << "\n";
        }

        // Update adjacency map (bidirectional link with cost n.cost)
        adjMap[routerIdKey_][key] = n.cost;
        adjMap[key][routerIdKey_] = n.cost;
    }
    else {
        // refresh existing neighbor timestamp and interface
        it->second.lastSeen = simTime();
        EV_DETAIL << "MecOspf: refreshed neighbor " << neighborIp << "\n";
    }

    // free packet
    delete packet;
}

/* === forwardPacket ===
 * Use L3 tag to get destination; find best route from Ipv4RoutingTable.
 * On forwarding: set InterfaceReq tag so ipOut sends via the chosen interface.
 */
void MecOspf::forwardPacket(Packet *packet)
{
    // Find destination info (L3AddressInd tag for received packets)
    auto l3ind = packet->findTag<L3AddressInd>();
    if (!l3ind) {
        EV_WARN << "MecOspf::forwardPacket: missing L3AddressInd tag; dropping\n";
        delete packet;
        return;
    }

    L3Address destAddr = l3ind->getDestAddress();
    if (destAddr.getType() != L3Address::IPv4) {
        EV_WARN << "MecOspf::forwardPacket: non-IPv4 dest; dropping\n";
        delete packet;
        return;
    }
    Ipv4Address dest = destAddr.toIpv4();

    if (!rt) {
        EV_WARN << "MecOspf::forwardPacket: no routing table available; dropping\n";
        delete packet;
        return;
    }

    // find best matching route
    const Ipv4Route *route = rt->findBestMatchingRoute(dest);
    if (!route) {
        EV_WARN << "MecOspf::forwardPacket: no route to " << dest << "; dropping\n";
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
    EV_INFO << "MecOspf::forwardPacket: forwarding to " << dest
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
        EV_WARN << "MecOspf: neighbor " << neighborIp << " timed out -> remove\n";
        neighbors.erase(key);
        rt->removeRoute(neighborRoutes[key]);
        delete neighborRoutes[key];
        neighborRoutes.erase(key);
    }

    // start building new topology
    clearIndirectRoutes();
    adjMap.clear();
    for (auto &kv : neighbors) {
        uint32_t nkey = kv.first;
        double cost = kv.second.cost;
        adjMap[routerIdKey_][nkey] = cost;
        adjMap[nkey][routerIdKey_] = cost;
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
        EV_INFO << "MecOspf::clearIndirectRoutes: none installed\n";
        return;
    }
    EV_INFO << "MecOspf::clearIndirectRoutes: removing " << indirectRoutes.size() << " routes\n";
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
        EV_INFO << "MecOspf::clearNeighborRoutes: none installed\n";
        return;
    }
    EV_INFO << "MecOspf::clearNeighborRoutes: removing " << neighborRoutes.size() << " routes\n";
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
        EV_WARN << "MecOspf::recomputeIndirectRouting: missing ift or rt\n";
        return;
    }

    EV_INFO << "MecOspf::recomputeIndirectRouting: starting SPF computation\n";

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
        cout << "MecOspf::handleStartOperation: OSPF starting up\n";

    EV_INFO << "MecOspf::handleStartOperation\n";
    clearIndirectRoutes();
    clearNeighborRoutes();

    simtime_t startupTime = par("startupTime");
    scheduleAfter(startupTime, helloTimer);
}

void MecOspf::handleStopOperation(LifecycleOperation *operation)
{
    EV_INFO << "MecOspf::handleStopOperation\n";
    cancelEvent(helloTimer);
    clearIndirectRoutes();
    clearNeighborRoutes();
    neighbors.clear();
    adjMap.clear();
}

void MecOspf::handleCrashOperation(LifecycleOperation *operation)
{
    EV_INFO << "MecOspf::handleCrashOperation\n";
    cancelEvent(helloTimer);
    clearIndirectRoutes();
    clearNeighborRoutes();
    neighbors.clear();
    adjMap.clear();
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
    EV_INFO << "MecOspf: finish() cleanup done\n";
}
