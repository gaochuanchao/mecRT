//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeFwdBase.cc / SchemeFwdBase.h
//
//  Description:
//    This file implements the basic scheduling scheme in the Mobile Edge Computing System with
//    backhaul network support, where tasks can be forwarded among RSUs after being offloaded to the access RSU.
//    The SchemeFwdBase class provides common functionalities for different scheduling schemes that support task forwarding.
//    By default, a greedy scheduling scheme is implemented.
//      [scheme source: C. Gao, A. Shaan and A. Easwaran, "Deadline-constrained Multi-resource Task Mapping 
//      and Allocation for Edge-Cloud Systems," GLOBECOM 2022, doi: 10.1109/GLOBECOM48099.2022.10001137.]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_SCHEDULER_SCHEME_GREEDY_BN_H_
#define _MECRT_SCHEDULER_SCHEME_GREEDY_BN_H_

#include "mecrt/apps/scheduler/SchemeBase.h"

class SchemeFwdGreedy : public SchemeBase
{
  protected:
    double virtualLinkRate_; // the rate of the virtual link in the backhaul network
    double fairFactor_; // the fairness factor limiting the maximum resource allocation, default is 1.0
    /***
     * Parameters used to store service instance information
     * each service instance is represented by four vectors: InstAppIds_, InstRsuIds_, InstBands_, InstCUs_
     * the vector index represents the instance ID, and the value represents the 
     *      application index, RSU index, resource blocks, computing units, offload RSU index, and process RSU index, respectively 
     */
    vector<int> instAppIndex_;  // application indices for the service instances
    vector<int> instRBs_;  // resource blocks for the service instances
    vector<int> instCUs_;  // computing units for the service instances
    vector<double> instUtility_;  // utility (i.e., energy savings) for the service instances
    vector<double> instMaxOffTime_;  // maximum allowable offloading time for the service instances
    vector<int> instOffRsuIndex_;  // offload RSU indices for each service instances
    vector<int> instProRsuIndex_;  // process RSU indices for each service instances

  public:
    SchemeFwdGreedy(Scheduler *scheduler);
    ~SchemeFwdGreedy()
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }

    /***
     * Initialize the scheduling data
     */
    virtual void initializeData();

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void generateScheduleInstances() override;

    /***
     * Schedule the request
     */
    virtual vector<srvInstance> scheduleRequests() override;

    /***
     * Compute the utility for a service instance
     * The default implementation is to return the energy savings
     */
    virtual double computeUtility(AppId &appId, double &offloadDelay, double &exeDelay, double &period);

    /***
     * Compute the data forwarding delay in the backhaul network based on the number of hops and data size
     */
    virtual double computeForwardingDelay(int hopCount, int dataSize);
};

#endif // _SCHEME_BASE_BN_H_