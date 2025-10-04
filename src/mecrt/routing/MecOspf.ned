//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    DynamicRouter.h / DynamicRouter.cc
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
package mecRT.mecrt.routing;

simple MecOspf
{
    parameters:
        volatile double startupTime @unit(s) = default(0.001s); // delay before starting OSPF
        double lsaWaitInterval @unit(s) = default(0.005s); // wait for LSA propagation
        double routeComputationDelay @unit(s) = default(0.01s); // wait before recomputing routes

        string interfaceTableModule = default("^.interfaceTable"); // path to the interface table
        string routingTableModule = default("^.ipv4.routingTable");     // path to the routing table
        string nodeInfoModulePath = default("^.nodeInfo"); // path to the node info module
        // string routingAlgorithm = default("dijkstra"); // you can extend later

        double helloInterval @unit(s) = default(5s);   // neighbor state detection interval
        double helloFeedbackDelay @unit(s) = default(0.003s); // delay waiting for hello feedback
        double neighborTimeout @unit(s) = default(11s); // timeout for neighbor inactivity

        @display("i=block/network2");                  // icon for visualization    
    
    gates:
        input ipIn @labels(Ipv4ControlInfo/up);
        output ipOut @labels(Ipv4ControlInfo/down);
}
