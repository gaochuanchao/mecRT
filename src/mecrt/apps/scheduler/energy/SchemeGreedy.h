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

#ifndef _MECRT_SCHEDULER_SCHEME_Greedy_H_
#define _MECRT_SCHEDULER_SCHEME_Greedy_H_

#include "mecrt/apps/scheduler/SchemeBase.h"

using namespace std;

// service instance represented by (appId, offloading rsuId, processing rsuId, bands, cmpUnits)
typedef tuple<AppId, MacNodeId, MacNodeId, int, int> srvInstance;

class Scheduler;

class SchemeGreedy : public SchemeBase
{
  protected:
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
    vector<double> instExeDelay_;  // execution delay for the service instances

  public:
    SchemeGreedy(Scheduler *scheduler);
    ~SchemeGreedy() 
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
    virtual vector<srvInstance> scheduleRequests() override;

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void generateScheduleInstances() override;

    /***
     * Compute execution delay for an application on a specific RSU
     */
    virtual double computeExeDelay(AppId appId, MacNodeId rsuId, double cmpUnits);
    
    /***
     * Compute the utility for a service instance
     * The default implementation is to return the energy savings
     */
    virtual double computeUtility(AppId &appId, double &offloadDelay, double &exeDelay, double &period);
};

#endif // _SCHEME_BASE_H_

