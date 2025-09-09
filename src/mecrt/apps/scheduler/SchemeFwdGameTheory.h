//
//                  VecSim
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the non-cooperative game theory scheduling scheme in resource scheduling
// consider task forwarding in the backhaul network
//

#ifndef _MECRT_SCHEDULER_SCHEME_FWD_GAME_THEORY_H_
#define _MECRT_SCHEDULER_SCHEME_FWD_GAME_THEORY_H_

#include "mecrt/apps/scheduler/SchemeFwdBase.h"

class SchemeFwdGameTheory : public SchemeFwdBase
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
