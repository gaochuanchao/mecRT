//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyFastSANF.cc / AccuracyFastSANF.h
//
//  Description:
//    This file implements the variance of FastSA without considering data forwarding in the backhaul network
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//


#ifndef _MECRT_SCHEDULER_SCHEME_ACCURACY_FAST_NO_FORWARDING_SA_H_
#define _MECRT_SCHEDULER_SCHEME_ACCURACY_FAST_NO_FORWARDING_SA_H_

#include "mecrt/apps/scheduler/accuracy/AccuracyGreedy.h"

class AccuracyFastSANF : public AccuracyGreedy
{
  protected:
    vector<string> instCategory_;
    vector<double> rbUtilization_;
    vector<double> cuUtilization_;

  public:
    AccuracyFastSANF(Scheduler *scheduler);
    ~AccuracyFastSANF() 
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }

    /***
     * Schedule the request
     */
    virtual vector<srvInstance> scheduleRequests() override;

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void generateScheduleInstances() override;

    virtual void defineInstanceCategory();

    virtual void candidateGenerateForType(vector<string> serviceTypes, vector<int>& instIndices, double& totalUtility);
};

#endif // _MECRT_SCHEDULER_SCHEME_ACCURACY_FAST_NO_FORWARDING_SA_H_