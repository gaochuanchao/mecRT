//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyFastIS.cc / AccuracyFastIS.h
//
//  Description:
//    This file implements the equivalently linear time approximation scheduling scheme 
//    in the Mobile Edge Computing System with backhaul network support.
//    In this scheme, we classify the service instances into four types:
//        LI: service instances light in terms of RB and CU (half or less of the available resources)
//        HI: service instances heavy in terms of RB or CU (more than half of the available resources)
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2026-03-20
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//


#ifndef _MECRT_SCHEDULER_SCHEME_ACCURACY_FAST_IS_H_
#define _MECRT_SCHEDULER_SCHEME_ACCURACY_FAST_IS_H_

#include "mecrt/apps/scheduler/accuracy/AccuracyGreedy.h"

class AccuracyFastIS : public AccuracyGreedy
{
  protected:
    vector<string> instCategory_;
    vector<double> rbUtilization_;
    vector<double> cuUtilization_;

  public:
    AccuracyFastIS(Scheduler *scheduler);
    ~AccuracyFastIS() 
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void generateScheduleInstances() override;

    /***
     * Initialize the scheduling data
     */
    virtual void initializeData() override;

    /***
     * Schedule the request
     */
    virtual vector<srvInstance> scheduleRequests() override;

    virtual void solutionGeneration(vector<int>& instIndices);
};

#endif // _MECRT_SCHEDULER_SCHEME_ACCURACY_FAST_IS_H_