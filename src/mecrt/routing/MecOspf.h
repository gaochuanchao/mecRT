//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecOspf.h / MecOspf.cc
//
//  Description:
//    This file implements a OSPF-like dynamic routing functionality in the edge server (RSU) in MEC.
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
//   High-Level Workflow:
//      1. Neighbor Discovery → Find who’s directly connected (Hello protocol).
//      2. Topology Maintenance → Build/update a link-state database (links, costs, neighbors).
//      3. Route Computation → Run Dijkstra on the database.
//      4. Forwarding → Use installed routes to send packets.
//      5. Fault Handling → Detect when neighbors disappear and re-run Dijkstra.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef __MECRT_MEC_OSPF_H
#define __MECRT_MEC_OSPF_H

#include "inet/common/INETDefs.h"
#include "inet/routing/base/RoutingProtocolBase.h"
#include "inet/networklayer/contract/IInterfaceTable.h"
#include "inet/networklayer/ipv4/Ipv4RoutingTable.h"
#include "inet/networklayer/ipv4/Ipv4Route.h"
#include <vector>
#include <unordered_map>

using namespace inet;
using namespace std;

class MecOspf : public RoutingProtocolBase
{
  protected:
    // neighbor entry discovered via Hello
    struct Neighbor {
        Ipv4Address destIp; // router ID of the neighbor (the address of the ipv4 module)
        NetworkInterface *interface = nullptr; // the outgoing interface to reach this neighbor
        double cost = 1.0; // link cost to this neighbor (default 1.0)
        simtime_t lastSeen = 0; // last Hello time

        string str() const {
            ostringstream os;
            os << "Neighbor " << destIp << " via " << (interface ? interface->getInterfaceName() : "nullptr")
               << " lastSeen=" << lastSeen << " cost=" << cost;
            return os.str();
        }

        // constructor
        Neighbor(const Ipv4Address& r, NetworkInterface *i, simtime_t t, double c) 
          : destIp(r), interface(i), cost(c), lastSeen(t)  {}

        Neighbor() = default;
    };

    // router id
    Ipv4Address routerId_;
    uint32_t routerIdKey_; // integer form of routerId_

    // INET helpers
    IInterfaceTable *ift = nullptr;
    Ipv4RoutingTable *rt = nullptr;

    // protocol state
    // keyed by int form of neighbor router ip
    map<uint32_t, Neighbor> neighbors; 
    // keyed by int form of router ip, store the network topology we know. 
    // e.g., {ip1: {ip2: cost, ip3: cost}, ip2: {ip1: cost, ...}, ...}
    map<uint32_t, map<uint32_t, double>> adjMap;
    // routes we installed (so we can remove them)
    map<uint32_t, Ipv4Route *> indirectRoutes;  // separate from routes to neighbors
    map<uint32_t, Ipv4Route *> neighborRoutes; // direct routes to neighbors

    // timers & config
    cMessage *helloTimer = nullptr;
    simtime_t helloInterval = 5;       // seconds
    simtime_t neighborTimeout = 15;    // seconds (dead interval)
    simtime_t lsFwdInterval = 0.001;     // seconds, link-state flooding interval (not used now)
    bool enableInitDebug_ = false;

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessageWhenUp(cMessage *msg) override;
    // lifecycle
    virtual void handleStartOperation(LifecycleOperation *operation) override;
    virtual void handleStopOperation(LifecycleOperation *operation) override;
    virtual void handleCrashOperation(LifecycleOperation *operation) override;

    virtual void finish() override;

    // protocol actions
    void sendHello();                                 // build/send Hello packets using tags
    void processHello(Packet *packet); // handle incoming Hello
    void forwardPacket(Packet *packet);               // forward normal packets using routing table
    void handleSelfTimer(cMessage *msg);                  // self-message handler (hello timer)

    // neighbor / failure
    void checkNeighborTimeouts();

    // routing / SPF
    void recomputeIndirectRouting();     // build graph, Dijkstra, install routes
    void clearIndirectRoutes();          // remove routes we previously added
    void clearNeighborRoutes();          // remove direct neighbor routes

    // helpers
    uint32_t ipKey(const Ipv4Address &a) const { return a.getInt(); }
    Ipv4Address getLocalAddressOnGate(cGate *gate);   // get local IP associated with a gate

  public:
    MecOspf();
    ~MecOspf();
};

#endif
