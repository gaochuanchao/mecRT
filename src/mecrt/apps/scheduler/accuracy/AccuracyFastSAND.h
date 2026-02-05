//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyFastSA.cc / AccuracyFastSA.h
//
//  Description:
//    This file implements the equivalently linear time approximation scheduling scheme 
//    in the Mobile Edge Computing System with backhaul network support.
//    In this scheme, we classify the service instances into four types:
//        LL: service instances light in terms of RB and CU (half or less of the available resources)
//        LH: service instances light in terms of RB but heavy in terms of CU
//        HL: service instances heavy in terms of RB but light in terms of CU
//        HH: service instances heavy in terms of RB and CU (more than half of the available resources)
//    either type LH or type HL will be considered separately
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//


#ifndef _MECRT_SCHEDULER_SCHEME_ACCURACY_FAST_SA_ND_H_
#define _MECRT_SCHEDULER_SCHEME_ACCURACY_FAST_SA_ND_H_

#include "mecrt/apps/scheduler/accuracy/AccuracyGreedy.h"

class AccuracyFastSAND : public AccuracyGreedy
{
  protected:
    vector<string> instCategory_;
    vector<double> rbUtilization_;
    vector<double> cuUtilization_;

  public:
    AccuracyFastSAND(Scheduler *scheduler);
    ~AccuracyFastSAND() 
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }

    /***
     * Schedule the request
     */
    virtual vector<srvInstance> scheduleRequests() override;

    virtual void defineInstanceCategory();

    virtual void candidateGenerateForType(vector<string> serviceTypes, vector<int>& instIndices);
};

#endif // _MECRT_SCHEDULER_SCHEME_ACCURACY_FAST_SA_ND_H_