//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyGameTheory.cc / AccuracyGameTheory.h
//
//  Description:
//    This file implements the Game Theory based scheduling scheme in the Mobile Edge Computing System.
//    The Game scheduling scheme is a non-cooperative game theory-based approach for resource scheduling,
//    which considers task forwarding in the backhaul network.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_SCHEDULER_SCHEME_ACCURACY_GAME_THEORY_BN_H_
#define _MECRT_SCHEDULER_SCHEME_ACCURACY_GAME_THEORY_BN_H_

#include "mecrt/apps/scheduler/accuracy/AccuracyGreedy.h"

class AccuracyGameTheory : public AccuracyGreedy
{
  public:
    AccuracyGameTheory(Scheduler *scheduler);
    ~AccuracyGameTheory()
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }

    virtual vector<srvInstance> scheduleRequests() override;
};

#endif // _MECRT_SCHEDULER_SCHEME_ACCURACY_GAME_THEORY_BN_H_