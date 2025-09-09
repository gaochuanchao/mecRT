//
//                  VecSim
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the quickLR scheduling scheme in the Vehicular Edge Computing System with Backhaul Network
//
// In this scheme, we classify the service instances into four types:
// 0: service instances light in terms of RB and CU (half or less of the available resources)
// 1: service instances light in terms of RB but heavy in terms of CU
// 2: service instances heavy in terms of RB but light in terms of CU
// 3: service instances heavy in terms of RB and CU (more than half of the available resources)
// either type 1 or type 2 will be considered separately
//

#ifndef _MECRT_SCHEDULER_SCHEME_FWD_QUICK_LR_H_
#define _MECRT_SCHEDULER_SCHEME_FWD_QUICK_LR_H_

#include "mecrt/apps/scheduler/SchemeFwdBase.h"

class SchemeFwdQuickLR : public SchemeFwdBase
{
  protected:
    int separateInstType_; // the instance type to be separated, 1 or 2, default is 1

  public:
    SchemeFwdQuickLR(Scheduler *scheduler);
    ~SchemeFwdQuickLR() 
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }

    /***
     * Schedule the request
     */
    virtual vector<srvInstance> scheduleRequests() override;

    virtual void candidateGenerateExcludeType(int instanceType, vector<int>& instIndices, double& totalUtility);
    virtual void candidateGenerateForType(int instanceType, vector<int>& instIndices, double& totalUtility);
};

#endif // _VEC_SCHEDULER_SCHEME_FWD_QUICK_LR_H_