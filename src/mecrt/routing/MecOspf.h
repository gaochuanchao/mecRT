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

        Neighbor(const Ipv4Address& r, NetworkInterface *i, simtime_t t, double c) 
          : destIp(r), interface(i), cost(c), lastSeen(t)  {}

        Neighbor() = default;
    };

	// the Link State Advertisement (LSA) info, maintained by each router
	struct LsaInfo {
		Ipv4Address origin;
		uint32_t seqNum;
		simtime_t installTime;
		map<Ipv4Address, double> neighbors;

		string str() const {
			ostringstream os;
			os << "LSA from " << origin << " seqNum=" << seqNum << " installed at " << installTime << "\n";
			for (const auto& n : neighbors) {
				os << "   neighbor " << n.first << " cost=" << n.second << "\n";
			}
			return os.str();
		}

		LsaInfo(const Ipv4Address& o, uint32_t s, simtime_t t) 
		  : origin(o), seqNum(s), installTime(t) {}

		LsaInfo() = default;
	};

	bool enableInitDebug_ = false;

    // router id
    Ipv4Address routerId_;
    uint32_t routerIdKey_; // integer form of routerId_
	uint32_t seqNum_ = 0; // LSA sequence number

    // INET helpers
    IInterfaceTable *ift = nullptr;
    Ipv4RoutingTable *rt = nullptr;

    // protocol state
    // keyed by int form of neighbor router ip
    map<uint32_t, Neighbor> neighbors; 
    // Link State Advertisement (LSA) database, store the lsa from all users, keyed by origin router ip
	// each router maintains its own LSA in the db, with seqNum indicating if it is updated
    map<Ipv4Address, LsaInfo> lsdb;  // key = ipKey(originRouter)
    // routes we installed (so we can remove them)
    map<uint32_t, Ipv4Route *> indirectRoutes;  // separate from routes to neighbors
    map<uint32_t, Ipv4Route *> neighborRoutes; // direct routes to neighbors

    // ======= Phrase 1: Neighbor Discovery =======
    cMessage *helloTimer = nullptr;
	cMessage *helloFeedbackTimer = nullptr; // not used now

	simtime_t helloFeedbackDelay = 0.003; // seconds, delay to send Hello in response to received Hello (not used now)
    simtime_t helloInterval = 5;       // seconds
    simtime_t neighborTimeout = 15;    // seconds (dead interval)
    
	// ======= Phrase 2: Start Broadcasting LSA =======
	cMessage *lsaTimer = nullptr; // not used now
	simtime_t lsaWaitInterval = 0.005; // seconds, wait time before sending LSA after neighbor discovery/change happens
    

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
	virtual void handleSelfTimer(cMessage *msg);                  // self-message handler (hello timer)

	// ====== Hello protocol ======
    virtual void sendInitialHello();                                 // build/send Hello packets using tags
	virtual void sendHelloFeedback(Packet *packet);               // send Hello in response to received Hello
    virtual void processHello(Packet *packet); // handle incoming Hello

	// ====== LSA protocol (not used now) ======
	virtual void sendLsa();                                        // build/send LSA packets using tags (not used now)
	virtual void processLsa(Packet *packet);                        // handle incoming LSA packets (not used
    virtual void forwardLsa(Packet *packet);               // forward normal packets using routing table
    

    // neighbor / failure
    virtual void checkNeighborTimeouts();

    // routing / SPF
    virtual void recomputeIndirectRouting();     // build graph, Dijkstra, install routes
    virtual void clearIndirectRoutes();          // remove routes we previously added
    virtual void clearNeighborRoutes();          // remove direct neighbor routes

    // helpers
    uint32_t ipKey(const Ipv4Address &a) const { return a.getInt(); }
    Ipv4Address getLocalAddressOnGate(cGate *gate);   // get local IP associated with a gate

  public:
    MecOspf();
    ~MecOspf();
};

#endif
