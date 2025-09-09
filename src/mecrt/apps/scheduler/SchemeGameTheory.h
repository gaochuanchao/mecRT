//
//                  VecSim
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the non-cooperative game theory scheduling scheme in resource scheduling
//

#ifndef _MECRT_SCHEDULER_SCHEME_GAME_THEORY_H_
#define _MECRT_SCHEDULER_SCHEME_GAME_THEORY_H_

#include "mecrt/apps/scheduler/SchemeBase.h"

class SchemeGameTheory : public SchemeBase
{
  public:
    SchemeGameTheory(Scheduler *scheduler);
    ~SchemeGameTheory() {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    };

    virtual vector<srvInstance> scheduleRequests() override;
};

#endif // _VEC_SCHEDULER_SCHEME_GAME_THEORY_H_
