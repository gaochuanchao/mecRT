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

#ifndef _MECRT_SCHEDULER_SCHEME_BASE_H_
#define _MECRT_SCHEDULER_SCHEME_BASE_H_

#include "mecrt/apps/scheduler/Scheduler.h"

using namespace std;

// service instance represented by (appId, offloading rsuId, processing rsuId, bands, cmpUnits)
typedef tuple<AppId, MacNodeId, MacNodeId, int, int> srvInstance;

class SchemeBase
{
  protected:
    // Protected members to access VecScheduler's data structures
    Scheduler *scheduler_;  // pointer to the VecScheduler instance
    Database *db_;  // pointer to the VecDatabase instance
    set<AppId> & unscheduledApps_;  // reference to the set of unscheduled applications
    map<AppId, RequestMeta> & appInfo_;  // reference to the application information
    map<MacNodeId, RsuResource> & rsuStatus_;  // reference to the RSU resource status
    map<MacNodeId, set<MacNodeId>> & vehAccessRsu_;  // reference to the vehicle access RSU mapping
    map<tuple<MacNodeId, MacNodeId>, int> & veh2RsuRate_;  // reference to the vehicle to RSU rate mapping
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
    map<AppId, string> appServiceType_; // map to store the service type for each application
    map<AppId, double> appExeDelay_;  // map to store the execution delay for each application

  public:
    SchemeBase(Scheduler *scheduler);
    ~SchemeBase() 
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }

    /***
     * Update reachable RSUs from each RSU based on the new backhaul network topology
     */
    virtual void updateReachableRsus(const map<MacNodeId, map<MacNodeId, double>>& topology);

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void generateScheduleInstances() {};

    /***
     * Schedule the request
     */
    virtual vector<srvInstance> scheduleRequests() { return vector<srvInstance>(); };

    /***
     * Compute offload delay for an application to a specific RSU
     */
    virtual double computeOffloadDelay(MacNodeId vehId, MacNodeId rsuId, int bands, int dataSize);

    /***
     * Get the utility value for each selected application
     */
    virtual double getAppUtility(AppId appId);

    /***
     * Get the execution delay for an application
     */
    virtual double getAppExeDelay(AppId appId);

    /***
     * Get the maximum allowable offloading time for an application
     */
    virtual double getMaxOffloadTime(AppId appId);

    /***
     * Get the service type for the scheduled application
     */
    virtual string getAppAssignedService(AppId appId);

};
#endif // _MECRT_SCHEDULER_SCHEME_BASE_H_