//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SchemeSARound.cc / SchemeSARound.h
//
//  Description:
//    This file implements a SARound scheduling scheme in the Mobile Edge Computing System.
//    The SARound scheduling scheme determines the service instance packing for each ES (RSU) one by one.
//    For each ES, it solves a linear programming (LP) relaxation of the original integer linear programming (ILP) problem.
//    Then, it applies a floor rounding technique to convert the fractional solution of the LP into an integer solution.
//      [scheme source: C. Gao and A. Easwaran, "Real-Time Service Subscription and Adaptive Offloading Control in
//       Vehicular Edge Computing", RTSS 2025]
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_SCHEDULER_SCHEME_SAROUND_H_
#define _MECRT_SCHEDULER_SCHEME_SAROUND_H_

#include "mecrt/apps/scheduler/SchemeBase.h"
#include "gurobi_c++.h"


class SchemeSARound : public SchemeBase
{
  protected:
    vector<double> reductPerAppIndex_;  // vector to store the reduction of utility for each application
    vector<vector<int>> instPerRsuIndex_;  // service instances for each RSU ID
    GRBEnv env_;  // Gurobi environment for solving LP problems

  public:
    SchemeSARound(Scheduler *scheduler);
    ~SchemeSARound() {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    };

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void generateScheduleInstances() override;

    virtual void initializeData() override;

    virtual vector<srvInstance> scheduleRequests() override;

    /***
     * determine the service instance candidates for each RSU
     */
    virtual vector<int> floorRounding(int rsuIndex, vector<double> & instUtilityTemp);

    /***
     * provide a dummy run to warm up the Gurobi environment
     */
    virtual void warmUpGurobiEnv();
    
};

#endif // _VEC_SCHEDULER_SCHEME_SAROUND_H_
