//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeBase.cc / SchemeBase.h
//
//  Description:
//    This file provides the basic functions for the global scheduler in the Mobile Edge Computing System.
//    Real implementation of different scheduling schemes should be derived from this base class.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/apps/scheduler/SchemeBase.h"

SchemeBase::SchemeBase(Scheduler *scheduler)
    : scheduler_(scheduler),
      db_(scheduler->db_),
      unscheduledApps_(scheduler->unscheduledApps_),
      appInfo_(scheduler->appInfo_),
      rsuStatus_(scheduler->rsuStatus_),
      vehAccessRsu_(scheduler->vehAccessRsu_),
      veh2RsuRate_(scheduler->veh2RsuRate_),
      rsuOnholdRbs_(scheduler->rsuOnholdRbs_),
      rsuOnholdCus_(scheduler->rsuOnholdCus_),
      ttiPeriod_(scheduler->ttiPeriod_),
      offloadOverhead_(scheduler->offloadOverhead_),
      cuStep_(scheduler->cuStep_),
      rbStep_(scheduler->rbStep_),
      srvTimeScale_(scheduler->srvTimeScale_),
      maxHops_(scheduler->maxHops_)
{
    EV << NOW << " SchemeBase::SchemeBase - Initialized" << endl;
}


void SchemeBase::updateReachableRsus(const map<MacNodeId, map<MacNodeId, double>>& topology)
{
    /***
     * store the RSU hop reachability, i.e., which RSUs can be reached from which RSU with maxHops_ hops
     * {
     *   {rsuId1, {rsuId2: hopCount, rsuId3: hopCount, ...}},
     *   {rsuId4, {rsuId5: hopCount, rsuId6: hopCount, ...}},
     *   ...
     * }
     * using BFS to find the reachable RSUs and their hop counts, store the result in reachableRsus_
     */
    EV << NOW << " SchemeBase::updateReachableRsus - update reachable RSUs with maxHops=" << maxHops_ << endl;

    reachableRsus_.clear();
    for (const auto& kv: topology)   // enumerate the topology
    {
        MacNodeId src = kv.first;
        queue<MacNodeId> q;
        map<MacNodeId, int> visited; // map to store the hop count for each RSU
        q.push(src);
        visited[src] = 0; // src is reachable from itself with 0 hops
        while (!q.empty())
        {
            MacNodeId currentRsu = q.front();
            q.pop();
            int currentHopCount = visited[currentRsu];
            // if the current hop count exceeds the maximum hops, stop the search
            if (currentHopCount >= maxHops_)
                continue;
            // iterate through the neighbors of the current RSU
            for (const auto& neighborKv : topology.at(currentRsu))
            {
                int neighbor = neighborKv.first;
                // if the neighbor is not visited, add it to the queue and mark it as visited
                if (visited.find(neighbor) == visited.end())
                {
                    visited[neighbor] = currentHopCount + 1; // increment the hop count
                    q.push(neighbor);
                }
            }
        }
        // Store the hop reachability information for the current RSU
        reachableRsus_[src] = visited;
    }

    // print the reachable RSUs for each RSU
    for (const auto& kv : reachableRsus_)
    {
        MacNodeId rsuId = kv.first;
        EV << "\tRSU[nodeId=" << rsuId << "] can reach:";
        for (const auto& neighborKv : kv.second)
        {
            MacNodeId neighborId = neighborKv.first;
            int hopCount = neighborKv.second;
            EV << " nodeId=" << neighborId << " (hop=" << hopCount << "),";
        }
        EV << endl;
    }
}


double SchemeBase::computeExeDelay(AppId appId, MacNodeId rsuId, double cmpUnits)
{
    /***
     * total computing cycle = T * C
     * where T is the execution time for the full computing resource allocation, and C is the capacity
     *      time = T * C / n
     * * where n is the number of computing units allocated to the application
     */

    //check if db_ is not null
    if (!db_) {
        std::cout << " SchemeBase::computeExeDelay - db_ is null, cannot compute execution delay" << endl;
    }
    // execution time for the full computing resource allocation
    double exeTime = db_->getGnbExeTime(appInfo_[appId].service, rsuStatus_[rsuId].deviceType);
    if (rsuStatus_[rsuId].cmpCapacity <= 0) {
        return INFINITY;
    }

    return exeTime * rsuStatus_[rsuId].cmpCapacity / cmpUnits;
}


double SchemeBase::computeOffloadDelay(MacNodeId vehId, MacNodeId rsuId, int bands, int dataSize)
{
    /***
     * during data transmission, there are multiple headers added to the data. In total, 33 bytes are added
     *      - UDP header (8B)
     *      - IP header (20B)
     *      - PdcpPdu header (1B)
     *      - RlcSdu header (2B) : RLC_HEADER_UM
     *      - MacPdu header (2B) : MAC_HEADER
     */
    double rate = veh2RsuRate_[make_tuple(vehId, rsuId)] * bands;  // byte per TTI
    double actualSize = dataSize + 33;
    int numFrames = ceil(actualSize / rate);

    return numFrames * ttiPeriod_;
}


double SchemeBase::getAppUtility(AppId appId)
{
    auto it = appUtility_.find(appId);
    if (it != appUtility_.end())
        return it->second;  // return the utility for the application
    else
        return 0.0;  // if not found, return 0
}


double SchemeBase::getMaxOffloadTime(AppId appId)
{
    auto it = appMaxOffTime_.find(appId);
    if (it != appMaxOffTime_.end())
        return it->second;  // return the maximum offloading time for the application
    else
        return 0.0;  // if not found, return 0
}


string SchemeBase::getAppAssignedService(AppId appId)
{
    auto it = appServiceType_.find(appId);
    if (it != appServiceType_.end())
        return it->second;  // return the service type for the application
    else
        return appInfo_[appId].service;  // if not found, return the original application type
}

