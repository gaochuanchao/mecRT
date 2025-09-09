//
//                  VecSim
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the Iterative scheduling scheme in resource scheduling
//
// the task mapping and resouce allocation problem is decomposed into two subproblems:
// 1. task mapping problem: determine where the service is deployed
// 2. resource allocation problem: determine how much resources are allocated to the service
// the two subproblems are solved iteratively until convergence
//

#ifndef _MECRT_SCHEDULER_SCHEME_ITERATIVE_H_
#define _MECRT_SCHEDULER_SCHEME_ITERATIVE_H_

#include "mecrt/apps/scheduler/SchemeBase.h"

class SchemeIterative : public SchemeBase
{
  protected:
    int maxIter_ = 0;  // maximum number of iterations for the iterative scheme
    vector<vector<int>> availMapping_;  // vector to store the available mapping for each application

    vector<int> appMapping_;  // vector to store the mapping to RSUs of each application
    vector<int> appRb_;  // vector to store the resource blocks allocated to each application
    vector<int> appCu_;  // vector to store the computing units allocated to each application
    vector<int> appInst_;  // vector to store the current instance index for each application
    vector<map<int, vector<int>>> instPerRSUPerApp_;  // {appIdx: {rsuIdx: {instIdx1, instIdx2, ...}}}, instances per RSU per application

  public:
    SchemeIterative(Scheduler *scheduler);
    ~SchemeIterative() {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    };

    virtual void initializeData() override;
    virtual void generateScheduleInstances() override;

    virtual vector<srvInstance> scheduleRequests() override;

    /***
     * determine the resource allocation for a given mapping
     */
    virtual void decideResourceAllocation();


    /***
     * determine mapping for a given resource allocation
     */
    virtual void decideMapping();
};

#endif // _VEC_SCHEDULER_SCHEME_ITERATIVE_H_
