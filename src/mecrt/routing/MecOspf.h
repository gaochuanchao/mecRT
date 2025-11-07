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

#include "mecrt/packets/routing/OspfLsa_m.h"
#include "mecrt/common/MecCommon.h"
#include "mecrt/common/NodeInfo.h"
#include "mecrt/apps/scheduler/Scheduler.h"

using namespace inet;
using namespace std;

class MecOspf : public omnetpp::cSimpleModule
{
  protected:
    // neighbor entry discovered via Hello
    struct Neighbor {
        Ipv4Address destIp; // router ID of the neighbor (the address of the ipv4 module)
        Ipv4Address gateway; // the IP address of the neighbor interface that our Hello arrived on
        NetworkInterface *outInterface = nullptr; // the outgoing interface to reach this neighbor
        double cost = 1.0; // link cost to this neighbor (default 1.0)
        simtime_t lastSeen = 0; // last Hello time

        string str() const {
            ostringstream os;
            os << "Neighbor " << destIp << " via " << (outInterface ? outInterface->getInterfaceName() : "nullptr")
               << " gateway=" << gateway << " lastSeen=" << lastSeen << " cost=" << cost;
            return os.str();
        }

        Neighbor(const Ipv4Address& r, const Ipv4Address& g, NetworkInterface *i, simtime_t t, double c) 
          : destIp(r), gateway(g), outInterface(i), cost(c), lastSeen(t)  {}

        Neighbor() = default;
    };

    // for Dijkstra algorithm
    struct Node {
        uint32_t nodeKey;   // node ID (router)
        double cost;        // tentative distance
        uint32_t prevHop;   // previous hop in the path
        bool visited;       // has the shortest path been finalized?
    };

	bool enableInitDebug_ = false;

    bool routeUpdate_ = true;

    int localPort_;
    inet::UdpSocket socket_;
    int socketId_ = -1;
    NodeInfo *nodeInfo_ = nullptr; // info about this node
    // router id
    Ipv4Address routerId_;
    uint32_t routerIdKey_; // integer form of routerId_
	uint32_t seqNum_ = 0; // LSA sequence number

    // INET helpers
    IInterfaceTable *ift_ = nullptr;
    Ipv4RoutingTable *rt_ = nullptr;

    // protocol state
    // keyed by int form of neighbor router ip
    map<uint32_t, Neighbor> neighbors_; 
    // Link State Advertisement (LSA) database, store the lsa from all users, keyed by origin router ip
	// each router maintains its own LSA in the db, with seqNum indicating if it is updated
    map<uint32_t, map<uint32_t, double>> topology_;  // key = ipKey(originRouter), {rsuAddr: {neighborAddr: cost}}
    map<uint32_t, MacNodeId> ipv4ToMacNodeId_; // mapping from ipv4 address to mac node id of the RSU, used for scheduler
    inet::Ptr<OspfLsa> selfLsa_;    // our own LSA packet, which will be maintained and updated by us
    map<uint32_t, inet::Ptr<OspfLsa>> lsaPacketCache_; // cache of recently received LSAs, keyed by origin router id
    // routes we installed (so we can remove them)
    map<uint32_t, Ipv4Route *> indirectRoutes_;  // separate from routes to neighbors
    map<uint32_t, Ipv4Route *> neighborRoutes_; // direct routes to neighbors

    // ======= Phrase 1: Neighbor Discovery =======
    cMessage *helloTimer_ = nullptr;
    simtime_t helloInterval_;       // seconds
    simtime_t neighborTimeout_;    // seconds (dead interval)

    bool neighborChanged_ = false; // flag to indicate neighbor change happened (if yes, need to send LSA)
    vector<uint32_t> newNeighbors_; // count of newly discovered neighbors
    
	// ======= Phrase 2: Start Broadcasting LSA =======
	cMessage *lsaTimer_ = nullptr; // not used now
	simtime_t lsaWaitInterval_ = 0.005; // seconds, wait time before sending LSA after neighbor discovery/change happens

    // ======= Phrase 3: Compute Routes to All Nodes in the Topology =======
    cMessage *routeComputationTimer_ = nullptr;
    simtime_t routeComputationDelay_ = 0.01; // seconds, wait time before recomputing routes after LSA update
    simtime_t largestLsaTime_ = 0; // track the largest LSA install time we have seen (to ensure LSA propagation is done before recomputing routes)

    // ======= Phrase 4: determine the scheduler node =======
    Ipv4Address schedulerAddr_ = Ipv4Address::UNSPECIFIED_ADDRESS;
    bool globalSchedulerReady_ = false; // whether the global scheduler is ready (if not, no cannot start scheduling)

  protected:
    virtual int numInitStages() const override { return NUM_INIT_STAGES; }
    virtual void initialize(int stage) override;
    virtual void handleMessage(cMessage *msg) override;
    // lifecycle
    // virtual void handleStartOperation(LifecycleOperation *operation) override;
    // virtual void handleStopOperation(LifecycleOperation *operation) override;
    // virtual void handleCrashOperation(LifecycleOperation *operation) override;

    virtual void finish() override;

    // protocol actions
	virtual void handleSelfTimer(cMessage *msg);                  // self-message handler (hello timer)

	// ====== Hello protocol ======
    virtual void sendInitialHello();                                 // build/send Hello packets using tags
	virtual void sendHelloFeedback(Packet *packet);               // send Hello in response to received Hello
    virtual void processHello(Packet *packet); // handle incoming Hello

	// ====== LSA protocol (not used now) ======
    virtual void handleLsaTimer();                                 // handle LSA timer firing
	virtual void updateLsaToNetwork();                             // update our own LSA to the whole network
	virtual void handleReceivedLsa(Packet *packet);                        // handle incoming LSA packets
    virtual void sendLsa(inet::Ptr<const OspfLsa> lsa, uint32_t neighborKey); // send LSA to a specific neighbor
    virtual void updateTopologyFromLsa(inet::Ptr<const OspfLsa>& lsa); // update our topology graph from a received LSA

    // neighbor / failure
    virtual void checkNeighborTimeouts();

    // routing / SPF
    virtual void recomputeIndirectRouting();     // build graph, Dijkstra, install routes
    virtual void dijkstra(uint32_t source, map<uint32_t, Node>& nodeInfos); // Dijkstra algorithm
    virtual void clearIndirectRoutes();          // remove routes we previously added
    virtual void clearNeighborRoutes();          // remove direct neighbor routes
    virtual void updateAdjListToScheduler(); // inform the scheduler about our current adjacency list

    // helpers
    uint32_t ipKey(const Ipv4Address &a) const { return a.getInt(); }
    Ipv4Address getLocalAddressOnGate(cGate *gate);   // get local IP associated with a gate
    virtual void resetGlobalScheduler(); // reset the global scheduler info

  public:
    MecOspf();
    ~MecOspf();

    virtual void handleNodeFailure(); // actions in case of node failure
};

#endif
