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

#include "inet/common/IProtocolRegistrationListener.h"
#include "inet/networklayer/common/NetworkInterface.h"

#include "inet/common/packet/Packet.h"
#include "inet/networklayer/common/L3AddressTag_m.h"
#include <inet/linklayer/common/InterfaceTag_m.h>
#include "inet/networklayer/common/HopLimitTag_m.h"
#include "inet/common/TagBase_m.h" // import inet.common.TagBase
#include "inet/common/ProtocolTag_m.h"
#include "inet/common/packet/Packet.h"

#include "inet/common/packet/chunk/ByteCountChunk.h"
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
    clearInstalledRoutes();
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
        // register protocol such that the IP layer can deliver packets to us
        registerProtocol(Protocol::ospf, gate("ipOut"), gate("ipIn"));

        // acquire required INET modules via parameters (StandardHost uses these params)
        try {
            ift = getModuleFromPar<IInterfaceTable>(par("interfaceTableModule"), this);
        } catch (cException &e) {
            EV_WARN << "MecOspf: cannot find interfaceTableModule param\n";
            ift = nullptr;
        }
        try {
            rt = getModuleFromPar<Ipv4RoutingTable>(par("routingTableModule"), this);
        } catch (cException &e) {
            EV_WARN << "MecOspf: cannot find routingTableModule param\n";
            rt = nullptr;
        }

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

    // incoming network packet: INET 4.5 uses Packet + tags
    Packet *packet = check_and_cast<Packet *>(msg);
    NetworkInterface *arrivalIf = ift->getInterfaceById(packet->findTag<InterfaceInd>()->getInterfaceId());
    if (!arrivalIf) {
        EV_WARN << "MecOspf: received packet on unknown interface\n";
        delete packet;
        return;
    }

    // We'll use L3AddressInd if present to extract source/destination; otherwise fallback.
    auto l3addrInd = packet->findTag<L3AddressInd>();
    // Identify Hello by presence of a packet name "HELLO" or a tag we set
    const char *pname = packet->getName();
    if (pname && strcmp(pname, "HELLO") == 0) {
        if (l3addrInd)
            EV_INFO << "MecOspf: HELLO received from " << l3addrInd->getSrcAddress().toIpv4() << "\n";
        else
            EV_INFO << "MecOspf: HELLO received (no L3 tag)\n";
        // process Hello - we pass ownership of packet into processHello (it will delete it)
        processHello(packet, arrivalIf);
        return;
    }

    // Otherwise treat as data packet to forward
    EV_INFO << "MecOspf: data packet received: " << (pname ? pname : "(unnamed)") << "\n";
    forwardPacket(packet);
}

// cleanup
void MecOspf::finish()
{
    if (helloTimer) {
        cancelAndDelete(helloTimer);
        helloTimer = nullptr;
    }
    clearInstalledRoutes();
    EV_INFO << "MecOspf: finish() cleanup done\n";
}

/* === sendHello ===
 * Create a Packet named "HELLO" and tag it with destination address and outgoing interface.
 * We set HopLimit and leave other tags default.
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
        hello->insertAtBack(makeShared<ByteCountChunk>(B(0))); // zero-length chunk, harmless

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
 * Receive a HELLO Packet; extract source IP (from L3AddressInd tag if present,
 * or from InterfaceInd + neighbor inferred). Update neighbor entry and localLinks.
 */
void MecOspf::processHello(Packet *packet, NetworkInterface *arrivalIf)
{
    // Attempt to get source address from L3AddressInd tag
    auto l3ind = packet->findTag<L3AddressInd>();
    Ipv4Address src = Ipv4Address::UNSPECIFIED_ADDRESS;

    if (l3ind) {
        // L3AddressInd provides canonical L3 info
        src = l3ind->getSrcAddress().toIpv4();
    }

    // If we didn't find src in tags, try to derive from arrival gate's interface
    if (src.isUnspecified()) {
        // some setups put source info in packet tags; if not, we can only use interface
        // We'll log and return — Hello without source is unusable
        EV_WARN << "MecOspf::processHello: no L3 tag found; cannot extract src IP (iface="
                << arrivalIf->getInterfaceName() << ")\n";
        delete packet;
        return;
    }

    // Insert or refresh neighbor entry
    uint32_t key = ipKey(src);
    Neighbor n;
    n.addr = src;
    n.interface = arrivalIf;
    n.lastSeen = simTime();

    auto it = neighbors.find(key);
    if (it == neighbors.end()) {
        // new neighbor
        neighbors.emplace(key, n);
        EV_INFO << "MecOspf: discovered neighbor " << src << " (neighbors=" << neighbors.size() << ")\n";
    }
    else {
        // refresh existing neighbor timestamp and interface
        it->second.lastSeen = simTime();
        it->second.interface = arrivalIf;
        EV_DETAIL << "MecOspf: refreshed neighbor " << src << "\n";
    }

    // Build a local link entry (we discovered src is reachable via this interface)
    Ipv4Address ifAddr = arrivalIf->getIpv4Address();
    if (!ifAddr.isUnspecified()) {
        Link L; L.u = ifAddr; L.v = src; L.metric = 1.0;
        // push_back unconditionally; duplicates are harmless for this simple demo
        localLinks.push_back(L);
        EV_INFO << "MecOspf: added local link " << ifAddr << " <-> " << src << "\n";
    }

    // Recompute routes when neighbor table changes
    recomputeRouting();

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

/* === checkNeighborTimeouts ===
 * Remove neighbors that haven't been heard from within neighborTimeout.
 * Trigger recomputeRouting() if anything changed.
 */
void MecOspf::checkNeighborTimeouts()
{
    std::vector<uint32_t> toRemove;
    for (auto &kv : neighbors) {
        const Neighbor &n = kv.second;
        if (simTime() - n.lastSeen > neighborTimeout) {
            toRemove.push_back(kv.first);
        }
    }
    if (toRemove.empty()) return;

    for (auto key : toRemove) {
        Ipv4Address addr = neighbors[key].addr;
        EV_WARN << "MecOspf: neighbor " << addr << " timed out -> remove\n";
        neighbors.erase(key);
        // also remove localLinks entries that include that neighbor
        localLinks.erase(std::remove_if(localLinks.begin(), localLinks.end(),
                             [&](const Link &L) { return L.v == addr || L.u == addr; }),
                         localLinks.end());
    }

    // recompute routes after topology change
    recomputeRouting();
}

/* === clearInstalledRoutes ===
 * Remove routes we previously installed (to avoid accumulating stale routes).
 */
void MecOspf::clearInstalledRoutes()
{
    if (!rt) return;
    if (installedRoutes.empty()) {
        EV_INFO << "MecOspf::clearInstalledRoutes: none installed\n";
        return;
    }
    EV_INFO << "MecOspf::clearInstalledRoutes: removing " << installedRoutes.size() << " routes\n";
    for (auto r : installedRoutes) {
        rt->removeRoute(r);
        delete r;
    }
    installedRoutes.clear();
}

/* === recomputeRouting ===
 * Build a graph from localLinks and neighbors (optionally LSAs if you extend)
 * Run Dijkstra from one of our local addresses and install host (/32) routes.
 */
void MecOspf::recomputeRouting()
{
    if (!ift || !rt) {
        EV_WARN << "MecOspf::recomputeRouting: missing ift or rt\n";
        return;
    }

    EV_INFO << "MecOspf::recomputeRouting: starting SPF computation\n";

    // 1) Build node list and index map
    std::unordered_map<uint32_t,int> idx;
    std::vector<Ipv4Address> nodes;

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

    // add neighbors
    for (auto &kv : neighbors) addNode(kv.second.addr);

    // add nodes from localLinks
    for (auto &L : localLinks) { addNode(L.u); addNode(L.v); }

    if (nodes.size() < 2) {
        EV_INFO << "MecOspf::recomputeRouting: not enough nodes (" << nodes.size() << "), nothing to do\n";
        return;
    }

    // 2) build adjacency list
    std::vector<std::vector<std::pair<int,double>>> adj(nodes.size());
    auto addEdge = [&](const Ipv4Address &a, const Ipv4Address &b, double w){
        int ia = idx[ipKey(a)];
        int ib = idx[ipKey(b)];
        adj[ia].push_back({ib,w});
        adj[ib].push_back({ia,w});
    };

    for (auto &L : localLinks) {
        // ensure nodes exist (they should)
        try {
            addEdge(L.u, L.v, L.metric);
        } catch (...) {
            // ignore
        }
    }

    // 3) choose source index: first local interface address that exists in nodes
    int srcIdx = -1;
    for (int i = 0; i < ift->getNumInterfaces(); ++i) {
        NetworkInterface *ie = ift->getInterface(i);
        if (!ie || ie->isLoopback()) continue;
        Ipv4Address a = ie->getIpv4Address();
        if (a.isUnspecified()) continue;
        uint32_t k = ipKey(a);
        if (idx.count(k)) { srcIdx = idx[k]; break; }
    }
    if (srcIdx < 0) {
        EV_WARN << "MecOspf::recomputeRouting: no local source address found\n";
        return;
    }
    Ipv4Address srcAddr = nodes[srcIdx];
    EV_INFO << "MecOspf::recomputeRouting: using source address " << srcAddr << "\n";

    // 4) Dijkstra
    const double INF = std::numeric_limits<double>::infinity();
    int N = nodes.size();
    std::vector<double> dist(N, INF);
    std::vector<int> prev(N, -1);
    using PQItem = std::pair<double,int>;
    std::priority_queue<PQItem, std::vector<PQItem>, std::greater<PQItem>> pq;

    dist[srcIdx] = 0.0;
    pq.push({0.0, srcIdx});
    while (!pq.empty()) {
        auto top = pq.top();
        double d = top.first;
        int u = top.second;
        pq.pop();
        
        if (d > dist[u]) continue;
        for (auto &edge : adj[u]) {
            int v = edge.first;
            double w = edge.second;
            if (dist[v] > dist[u] + w) {
                dist[v] = dist[u] + w;
                prev[v] = u;
                pq.push({dist[v], v});
            }
        }
    }

    // 5) Install routes: remove previous dynamic routes, then add new ones
    clearInstalledRoutes();

    for (int i = 0; i < N; ++i) {
        if (i == srcIdx) continue;
        if (dist[i] == INF) continue; // unreachable

        // find next hop: walk back from i to the node whose predecessor is srcIdx
        int walker = i;
        int before = prev[walker];
        if (before == -1) {
            // possibly direct neighbor with no prev recorded — find direct link
            bool direct = false;
            for (auto &e : adj[srcIdx]) if (e.first == i) { direct = true; break; }
            if (!direct) continue;
            // nextHop is nodes[i]
            Ipv4Address nextHop = nodes[i];
            // find outgoing interface: neighbor entry for nextHop
            NetworkInterface *outIf = nullptr;
            auto itn = neighbors.find(ipKey(nextHop));
            if (itn != neighbors.end()) {
                outIf = itn->second.interface;
            }
            if (!outIf) continue;
            // create route to nodes[i] via nextHop
            Ipv4Route *r = new Ipv4Route();
            r->setDestination(nodes[i]);
            r->setNetmask(Ipv4Address::ALLONES_ADDRESS);
            r->setGateway(nextHop);
            r->setInterface(outIf);
            r->setSourceType(Ipv4Route::OSPF);
            r->setMetric(1);
            rt->addRoute(r);
            installedRoutes.push_back(r);
            EV_INFO << "Installed direct route: " << nodes[i] << " via " << nextHop << "\n";
            continue;
        }

        // walk back until predecessor is srcIdx
        while (prev[walker] != srcIdx && prev[walker] != -1) walker = prev[walker];
        if (prev[walker] != srcIdx) continue;
        Ipv4Address nextHop = nodes[walker];
        Ipv4Address dest = nodes[i];

        // determine outgoing interface by looking up neighbor entry for nextHop
        NetworkInterface *outIf = nullptr;
        auto itn = neighbors.find(ipKey(nextHop));
        if (itn != neighbors.end()) {
            outIf = itn->second.interface;
        }
        if (!outIf) {
            EV_WARN << "MecOspf::recomputeRouting: no outIf for nextHop " << nextHop << ", skip route for " << dest << "\n";
            continue;
        }

        // create IPv4 route object and add to routing table
        Ipv4Route *route = new Ipv4Route();
        route->setDestination(dest);
        route->setNetmask(Ipv4Address::ALLONES_ADDRESS); // /32
        route->setGateway(nextHop);
        route->setInterface(outIf);
        route->setSourceType(Ipv4Route::OSPF);
        route->setMetric(1);

        rt->addRoute(route);
        installedRoutes.push_back(route);

        EV_INFO << "Installed route dest=" << dest << " via " << nextHop
                                    << " dev=" << outIf->getInterfaceName() << "\n";
    }

    EV_INFO << "MecOspf::recomputeRouting: finished (installed routes=" << installedRoutes.size() << ")\n";
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
        std::cout << "MecOspf::handleStartOperation: OSPF starting up\n";

    EV_INFO << "MecOspf::handleStartOperation\n";

    simtime_t startupTime = par("startupTime");
    scheduleAfter(startupTime, helloTimer);
}

void MecOspf::handleStopOperation(LifecycleOperation *operation)
{
    EV_INFO << "MecOspf::handleStopOperation\n";
    cancelEvent(helloTimer);
    clearInstalledRoutes();
    neighbors.clear();
    localLinks.clear();
}

void MecOspf::handleCrashOperation(LifecycleOperation *operation)
{
    EV_INFO << "MecOspf::handleCrashOperation\n";
    cancelEvent(helloTimer);
    clearInstalledRoutes();
    neighbors.clear();
    localLinks.clear();
}

