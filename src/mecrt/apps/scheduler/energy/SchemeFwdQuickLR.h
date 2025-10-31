//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeFwdQuickLR.cc / SchemeFwdQuickLR.h
//
//  Description:
//    This file implements the equivalently linear time approximation scheduling scheme 
//    in the Mobile Edge Computing System with backhaul network support.
//    In this scheme, we classify the service instances into four types:
//        0: service instances light in terms of RB and CU (half or less of the available resources)
//        1: service instances light in terms of RB but heavy in terms of CU
//        2: service instances heavy in terms of RB but light in terms of CU
//        3: service instances heavy in terms of RB and CU (more than half of the available resources)
//    either type 1 or type 2 will be considered separately
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//


#ifndef _MECRT_SCHEDULER_SCHEME_FWD_QUICK_LR_H_
#define _MECRT_SCHEDULER_SCHEME_FWD_QUICK_LR_H_

#include "mecrt/apps/scheduler/energy/SchemeFwdGreedy.h"

class SchemeFwdQuickLR : public SchemeFwdGreedy
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