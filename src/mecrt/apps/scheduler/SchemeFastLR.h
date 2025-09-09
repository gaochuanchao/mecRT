//
//                  VecSim
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the FastLR scheduling scheme in resource scheduling
//

#ifndef _MECRT_SCHEDULER_SCHEME_FAST_LR_H_
#define _MECRT_SCHEDULER_SCHEME_FAST_LR_H_

#include "mecrt/apps/scheduler/SchemeBase.h"

class SchemeFastLR : public SchemeBase
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


