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

#ifndef _MECRT_SCHEDULER_SCHEME_FAST_LR_H_
#define _MECRT_SCHEDULER_SCHEME_FAST_LR_H_

#include "mecrt/apps/scheduler/energy/SchemeGreedy.h"

class SchemeFastLR : public SchemeGreedy
{

  public:
    SchemeFastLR(Scheduler *scheduler);
    ~SchemeFastLR() {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    };

    virtual vector<srvInstance> scheduleRequests() override;
};

#endif // _VEC_SCHEDULER_SCHEME_FAST_LR_H_


