//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeFwdGameTheory.cc / SchemeFwdGameTheory.h
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

#ifndef _MECRT_SCHEDULER_SCHEME_FWD_GAME_THEORY_H_
#define _MECRT_SCHEDULER_SCHEME_FWD_GAME_THEORY_H_

#include "mecrt/apps/scheduler/energy/SchemeFwdGreedy.h"

class SchemeFwdGameTheory : public SchemeFwdGreedy
{
  public:
    SchemeFwdGameTheory(Scheduler *scheduler);
    ~SchemeFwdGameTheory()
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }

    virtual vector<srvInstance> scheduleRequests() override;
};

#endif // _VEC_SCHEDULER_SCHEME_FWD_GAME_THEORY_H_
