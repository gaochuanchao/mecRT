//
//                  VecSim
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the base scheduling scheme in the Vehicular Edge Computing System with Backhaul Network
//

#ifndef _MECRT_SCHEDULER_SCHEME_BASE_BN_H_
#define _MECRT_SCHEDULER_SCHEME_BASE_BN_H_

#include "mecrt/apps/scheduler/SchemeBase.h"

class SchemeFwdBase : public SchemeBase
{
  protected:
    double virtualLinkRate_; // the rate of the virtual link in the backhaul network
    double fairFactor_; // the fairness factor limiting the maximum resource allocation, default is 1.0
    /***
     * Additional parameters used to store service instance information
     * each service instance is represented by four vectors: InstAppIds_, InstOffRsuIds_, InstProcRsuIds_, InstBands_, InstCUs_
     * the vector index represents the instance index, and the value represents the 
     *      offload RSU index, process RSU index
     */
    vector<int> instOffRsuIndex_;  // offload RSU indices for each service instances
    vector<int> instProRsuIndex_;  // process RSU indices for each service instances

  public:
    SchemeFwdBase(Scheduler *scheduler);
    ~SchemeFwdBase() 
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
     * Schedule the request
     */
    virtual vector<srvInstance> scheduleRequests() override;

    /***
     * Compute the data forwarding delay in the backhaul network based on the number of hops and data size
     */
    virtual double computeForwardingDelay(int hopCount, int dataSize);
};

#endif // _SCHEME_BASE_BN_H_