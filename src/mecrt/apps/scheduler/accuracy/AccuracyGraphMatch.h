//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyGraphMatch.cc / AccuracyGraphMatch.h
//
//  Description:
//    This file implements the Graph Matching based scheduling scheme in the Mobile Edge Computing System with
//    backhaul network support.
//    The Graph Matching scheduling scheme transform the resource allocation problem into a maximum weight
//    three-dimensional matching problem.
//      [scheme source: C. Gao and A. Easwaran, "Energy-Efficient Joint Offloading and Resource Allocation
//      for Deadline-Constrained Tasks in Multi-Access Edge Computing", RTCSA 2025]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_SCHEDULER_SCHEME_ACCURACY_GRAPH_MATCH_BN_H_
#define _MECRT_SCHEDULER_SCHEME_ACCURACY_GRAPH_MATCH_BN_H_

#include "mecrt/apps/scheduler/accuracy/AccuracyGreedy.h"
#include "gurobi_c++.h"

class AccuracyGraphMatch : public AccuracyGreedy
{

  protected:
    /***
     * Bipartite graph structure for the offload and processing RSU nodes
     * The graph is represented as two sets of nodes (application nodes and RSU nodes) and
     * a set of edges connecting the application nodes to the RSU nodes.
     * Each edge has a resource demand associated with it.
     * 
     * each app node corresponds to an application index
     * each RSU node corresponds to a pair of (RSU index, RSU rank)
     */
    struct BipartiteGraph
    {
        set<int> appNodeSet;  // app nodes in the first partition
        vector<int> appNodeVec;  // vector to store the application index of each app node, {app index}
        map<int, int> appNode2VecIdx;  // map to store the application index to node vector index mapping
        set<array<int, 2>> rsuNodeSet;  // RSU nodes in the second partition, {rsu index, rank}
        vector<array<int, 2>> rsuNodeVec;  // vector to store the RSU nodes
        
        set<array<int, 2>> edgeSet;    // {{app vector index, rsu node vector index}}
        vector<array<int, 2>> edgeVec;  // store the vector of edges
        vector<double> resDemand;  // resource demand for each edge
    };


    struct TripartiteGraph
    {
        vector<int> appNodeVec;  // vector to store the application index of each app node, {app index}
        vector<array<int, 2>> offRsuNodeVec;  // offload RSU nodes in the second partition, {rsu index, rank}
        vector<array<int, 2>> proRsuNodeVec;  // process RSU nodes in the third partition, {rsu index, rank}

        set<array<int, 3>> edgeSet;		// {(app vector index, offload rsu vector index, process rsu vector index)}
        vector<array<int, 3>> edgeVec;  // store the list of hyper edges
        vector<int> rbDemand;  // bandwidth resource demand for each edge index, {edge vector index: demand}
        vector<int> cuDemand;  // computational resource demand for each edge index, {edge vector index: demand}
        vector<double> weight;    // utility value of each edge index, {edge vector index: weight}

        vector<vector<int>> edges4App;  // {app vector index: vector of hyper edges indices}
        vector<vector<int>> edges4OffRsu;  // {offload RSU node vector index : vector of hyper edges indices}
        vector<vector<int>> edges4ProRsu;  // {processing RSU node vector index : vector of hyper edges indices}
    };

    // Additional parameters specific to the graph matching scheme can be added here
    vector<vector<int>> instPerOffRsuIndex_;  // store the instances per offload RSU index
    vector<vector<int>> instPerProRsuIndex_;  // store the instances per processing RSU index
    vector<vector<int>> instPerAppIndex_;  // store the instances per application index
    GRBEnv env_;  // Gurobi environment for solving LP problems

  public:
    AccuracyGraphMatch(Scheduler *scheduler);
    ~AccuracyGraphMatch() 
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }

    /***
     * Initialize the scheduling data
     */
    virtual void initializeData() override;

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void generateScheduleInstances() override;

    /***
     * Schedule the request using graph matching algorithm
     */
    virtual vector<srvInstance> scheduleRequests() override;

    virtual void solvingLP(map<int, double> & lpSolution);

    virtual void constructBipartiteGraph(BipartiteGraph& biGraph, map<int, vector<int>> & instIdx2Edge, map<int, double>& lpSolution, bool isOffload);

	virtual void mergeBipartiteGraphs(TripartiteGraph& triGraph, const BipartiteGraph& offGraph, const map<int, vector<int>>& instIdx2OffEdge, 
		const BipartiteGraph& proGraph, const map<int, vector<int>>& instIdx2ProEdge, map<int, double>& lpSolution);

	virtual void solvingRelaxedTripartiteGraphMatching(TripartiteGraph& triGraph, map<int, double>& lpSolution);
	
	virtual vector<srvInstance> fractionalLocalRatioMethod(TripartiteGraph& triGraph, map<int, double>& lpSolution);

    /***
     * Provide a dummy run to warm up the Gurobi environment
     */
    virtual void warmUpGurobiEnv();
};

#endif
