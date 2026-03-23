//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    AccuracyDistIS.cc / AccuracyDistIS.h
//
//  Description:
//    This file provides the implementation of the AccuracyDistIS scheduling scheme, which is a 
//    distributed scheduling scheme for accuracy optimization in the Mobile Edge Computing System.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2026-03-22
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_SCHEDULER_SCHEME_ACCURACY_DISTIS_H_
#define _MECRT_SCHEDULER_SCHEME_ACCURACY_DISTIS_H_

#include "mecrt/apps/scheduler/SchemeBase.h"

class AccuracyDistIS : public SchemeBase
{
  protected:
    MacNodeId rsuId_; // the RSU/gNB ID of the scheduler node
    int maxRB_; // the resource block capacity of the RSUs
    int maxCU_; // the computing unit capacity of the RSUs
    /***
     * Parameters used to store service instance information
     * each service instance is represented by four vectors: InstAppIds_, InstRsuIds_, InstBands_, InstCUs_
     * the vector index represents the instance ID, and the value represents the 
     *      application index, RSU index, resource blocks, computing units, offload RSU index, and process RSU index, respectively 
     */
    vector<int> instAppIndex_;  // application indices for the service instances
    vector<int> instAppBeginIndex_;  // the begin index of an application's service instances in instAppIndex_
    vector<int> instAppEndIndex_;  // the end index (exclusive) of an application's service instances in instAppIndex_
    vector<int> instRBs_;  // resource blocks for the service instances
    vector<int> instCUs_;  // computing units for the service instances
    vector<double> instUtility_;  // utility (e.g., performance accuracy) for the service instances
    vector<double> instMaxOffTime_;  // maximum allowable offloading time for the service instances
    vector<string> instServiceType_;  // selected service types for each service instances
    vector<double> instExeDelay_;  // execution delay for each service instance

    vector<string> instCategory_; // category for the service instances
    vector<double> rbUtilization_;  // resource block utilization for the service instances
    vector<double> cuUtilization_;  // computing unit utilization for the service instances

    double reductionRsu_; // reduction for the RSU
    vector<double> reductAppInRsu_; // vector to store the reduction of utility for each application in the RSU

    vector<int> candidateInsts_; // store the index of the candidate service instances
    vector<srvInstance> finalSchedule_; // store the final schedule for the distributed scheduling scheme

  public:
    AccuracyDistIS(Scheduler *scheduler);
    ~AccuracyDistIS()
    {
        scheduler_ = nullptr;  // reset the pointer to avoid dangling pointer
        db_ = nullptr;  // reset the pointer to avoid dangling pointer
    }


    /***
     * Initialize the scheduling data
     */
    virtual void initializeData();

    /***
     * Generate schedule instances based on the pending applications and the available resources
     */
    virtual void generateScheduleInstances() override;


    /***
     * select the candidates (e.g., schedule instances) for target applications.
     * in distributed scheduling, every scheduler schedule applications in batches, one batch for each synchronization round
     * targetApps: {appId: utilityReduction}  the utility reduction for the target applications
     * targetCategory: the category of candidates to select
     * return: {appId: utilityReduction} the updated utility reduction for the target applications
     */
    virtual map<MacNodeId, double> candidateSelection(map<MacNodeId, double>& targetApps, string targetCategory) override;

    /***
     * select the final solution from the candidates for target applications.
     * in distributed scheduling, the solution is also selected in batches, one batch for each synchronization round
     * targetApps: {appId: isScheduled}  whether the target applications has been scheduled
     * targetCategory: the category of candidates to select
     * return: {appId: isScheduled} the updated scheduling result for the target applications
     */
   virtual map<MacNodeId, bool> solutionSelection(map<MacNodeId, bool>& targetApps, string targetCategory) override;

    /***
     * obtain solution when the distributed scheduling scheme completes
     */
    virtual vector<srvInstance> getFinalSchedule() override {return finalSchedule_; };


    /***
     * Compute execution delay for an application on a specific RSU
     */
    virtual double computeExeDelay(MacNodeId rsuId, double cmpUnits, string serviceType);

    virtual int computeMinRequiredCUs(MacNodeId rsuId, double exeTimeThreshold, string serviceType);
    virtual int computeMinRequiredRBs(MacNodeId vehId, MacNodeId rsuId, double offloadTimeThreshold, int dataSize);

    /***
     * Compute the utility for a service instance
     * The default implementation is to return the energy savings
     */
    virtual double computeUtility(AppId appId, string serviceType);
};


#endif // _MECRT_SCHEDULER_SCHEME_ACCURACY_DistIS_H_



