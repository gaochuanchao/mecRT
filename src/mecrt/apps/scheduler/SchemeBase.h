//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeBase.cc / SchemeBase.h
//
//  Description:
//    This file implements the basic scheduling scheme in the Mobile Edge Computing System.
//    The SchemeBase class provides common functionalities for different scheduling schemes,
//    such as data initialization, service instance generation, and utility computation.
//    By default, a greedy scheduling scheme is implemented.
//      [scheme source: C. Gao, A. Shaan and A. Easwaran, "Deadline-constrained Multi-resource Task Mapping 
//      and Allocation for Edge-Cloud Systems," GLOBECOM 2022, doi: 10.1109/GLOBECOM48099.2022.10001137.]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_SCHEDULER_SCHEME_BASE_H_
#define _MECRT_SCHEDULER_SCHEME_BASE_H_

#include "mecrt/apps/scheduler/Scheduler.h"

using namespace std;

// service instance represented by (appId, offloading rsuId, processing rsuId, bands, cmpUnits)
typedef tuple<AppId, MacNodeId, MacNodeId, int, int> srvInstance;

class SchemeBase
{
    friend class Scheduler;

  protected:
    // Protected members to access VecScheduler's data structures
    Scheduler *scheduler_;  // pointer to the VecScheduler instance
    Database *db_;  // pointer to the VecDatabase instance
    set<AppId> & unscheduledApps_;  // reference to the set of unscheduled applications
    map<AppId, RequestMeta> & appInfo_;  // reference to the application information
    map<MacNodeId, RsuResource> & rsuStatus_;  // reference to the RSU resource status
    map<MacNodeId, set<MacNodeId>> & vehAccessRsu_;  // reference to the vehicle access RSU mapping
    map<tuple<MacNodeId, MacNodeId>, int> & veh2RsuRate_;  // reference to the vehicle to RSU rate mapping
    map<tuple<MacNodeId, MacNodeId>, omnetpp::simtime_t> & veh2RsuTime_;  // reference to the vehicle to RSU time mapping
    map<MacNodeId, int> & rsuOnholdRbs_;    // reference to the RSU onhold resource blocks
    map<MacNodeId, int> & rsuOnholdCus_;    // reference to the RSU onhold computing units
    double ttiPeriod_ = 0.001; // duration for each TTI
    double offloadOverhead_ = 0;    // the overhead for offloading
    int cuStep_ = 1;  // the step for computing units, default is 1
    int rbStep_ = 1;  // the step for resource blocks, default is 1
    double srvTimeScale_ = 1.0; // the scale for app execution time on servers with full resource, default is 1.0
    int maxHops_ = 1; // the maximum number of hops for task forwarding in the backhaul network, default is 1
    map<MacNodeId, map<MacNodeId, int>> reachableRsus_; // {srcRsu: {reachableRsu: hops}}, the reachable RSUs within maxHops_

    /***
     * Protected members for scheduling
     * Id refers to the original application ID and RSU ID
     * Idx refers to the index in the vector, which is used to represent the real application ID and RSU ID for simplicity
     */
    vector<AppId> appIds_;  // using vector index number to represent the real application ID
    map<AppId, int> appId2Index_;  // map to store the application ID to index mapping
    vector<MacNodeId> rsuIds_;  // using vector index number to represent the real RSU IDs
    map<MacNodeId, int> rsuId2Index_;  // map to store the RSU ID to index mapping
    vector<int> rsuRBs_;  // using vector index number to represent the resource blocks of the RSUs
    vector<int> rsuCUs_;  // using vector index number to represent the computing units of the RSUs
    map<AppId, double> appMaxOffTime_;  // map to store the maximum offloading time for each application
    map<AppId, double> appUtility_;  // map to store the utility (i.e., energy savings) for each application

    /***
     * Parameters used to store service instance information
     * each service instance is represented by four vectors: InstAppIds_, InstRsuIds_, InstBands_, InstCUs_
     * the vector index represents the instance ID, and the value represents the 
     *      application index, RSU index, resource blocks, and computing units respectively
     */
    vector<int> instAppIndex_;  // application indices for the service instances
    vector<int> instRsuIndex_;  // RSU indices for the service instances
    vector<int> instRBs_;  // resource blocks for the service instances
    vector<int> instCUs_;  // computing units for the service instances
    vector<double> instUtility_;  // utility (i.e., energy savings) for the service instances
    vector<double> instMaxOffTime_;  // maximum allowable offloading time for the service instances

  public:
    SchemeBase(Scheduler *scheduler);
    ~SchemeBase() 
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }


    /***
     * Initialize the scheduling data
     * This function should be called before scheduling requests
     */
    virtual void initializeData();

    /***
     * Schedule the request
     */
    virtual vector<srvInstance> scheduleRequests();

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void generateScheduleInstances();
    
    /***
     * Compute offload delay for an application to a specific RSU
     */
    virtual double computeOffloadDelay(MacNodeId vehId, MacNodeId rsuId, int bands, int dataSize);
    
    /***
     * Compute execution delay for an application on a specific RSU
     */
    virtual double computeExeDelay(AppId appId, MacNodeId rsuId, double cmpUnits);

    /***
     * Compute the utility for a service instance
     * The default implementation is to return the energy savings
     */
    virtual double computeUtility(AppId &appId, double &offloadDelay, double &exeDelay, double &period);


    /***
     * Get the maximum allowable offloading time for an application
     */
    virtual double getMaxOffloadTime(AppId appId);


    /***
     * Get the utility value for each selected application
     */
    virtual double getAppUtility(AppId appId);


    /***
     * Update reachable RSUs from each RSU based on the new backhaul network topology
     */
    virtual void updateReachableRsus(const map<MacNodeId, map<MacNodeId, double>>& topology);
};

#endif // _SCHEME_BASE_H_

