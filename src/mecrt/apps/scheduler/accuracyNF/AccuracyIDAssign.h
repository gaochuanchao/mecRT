//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeFastLR.cc / SchemeFastLR.h
//
//  Description:
//    This file implements the FastLR scheduling scheme in the Mobile Edge Computing System.
//    The FastLR scheme is a linear time approximation algorithm for the multi-resource scheduling problem,
//    which categorizes all candidate service instances into two groups based on their resource demands,
//    and prioritizes the light instances (with resource demand no more than half) over the heavy instances.
//     [Scheme Source: Chuanchao Gao and Arvind Easwaran. 2025. Local Ratio based Real-time Job Offloading 
//      and Resource Allocation in Mobile Edge Computing. In Proceedings of the 4th International Workshop 
//      on Real-time and IntelliGent Edge computing (RAGE '25). Association for Computing Machinery, New York, 
//      NY, USA, Article 6, 1–6. https://doi.org/10.1145/3722567.3727843]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_SCHEDULER_SCHEME_ACCURACY_NF_IDASSIGN_H_
#define _MECRT_SCHEDULER_SCHEME_ACCURACY_NF_IDASSIGN_H_

#include "mecrt/apps/scheduler/accuracy/AccuracyGreedy.h"

class AccuracyIDAssign : public AccuracyGreedy
{
  protected:
    vector<double> instMaxUtilization_;  // vector to store the maximum resource utilization for each instance
    vector<double> instUtilizationSum_; // vector to store the sum of resource utilization for each instance
    vector<vector<int>> instPerApp_;  // {appIdx: {instIdx1, instIdx2, ...}}, instances per application
    vector<vector<int>> instPerRsu_;  // {rsuIdx: {instIdx1, instIdx2, ...}}, instances per RSU

  public:
    AccuracyIDAssign(Scheduler *scheduler);
    ~AccuracyIDAssign() {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    };

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void initializeData() override;
    virtual void generateScheduleInstances() override;

    virtual vector<srvInstance> scheduleRequests() override;
};

#endif // _MECRT_SCHEDULER_SCHEME_ACCURACY_NF_IDASSIGN_H_


