//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    FDSchemeUl.cc / FDSchemeUl.h
//
//  Description:
//    This file implements the uplink scheduling scheme for the gNB in the MEC.
//    This scheme prioritizes the UE with the best CQI in each time slot.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_GNB_FDSCHEMEUL_H_
#define _MECRT_GNB_FDSCHEMEUL_H_

#include "stack/mac/scheduler/LteScheduler.h"
#include "mecrt/nic/mac/scheduler/GnbSchedulerUl.h"

class FDSchemeUl : public virtual LteScheduler
{
  protected:

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

    /// Associated GnbSchedulerUl (it is the one who creates the FDSchemeUl)
    GnbSchedulerUl* eNbScheduler_;

  public:

    virtual void prepareSchedule() override;

    virtual void commitSchedule() override;

    /**
     * Initializes the GnbSchedulerUl.
     * @param gNbSchedulerUl gNb uplink scheduler
     */
    virtual void setGnbSchedulerUl(GnbSchedulerUl* gNbSchedulerUl) { eNbScheduler_ =  gNbSchedulerUl;}

    /// performs request of grant to the eNbScheduler
    virtual unsigned int requestGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible , std::vector<BandLimit>* bandLim = nullptr) override;

    virtual void updateSchedulingInfo() override {}

  protected:

    /**
     * The schedule function is splitted in two phases
     *  - in the first phase, carried out by the prepareSchedule(),
     *    the computation of the algorithm on temporary structures is performed
     *  - in the second phase, carried out by the storeSchedule(),
     *    the commitment to the permanent variables is performed
     *
     * In this way, if in the environment there's a special module which wants to execute
     * more schedulers, compare them and pick a single one, the execSchedule() of each
     * scheduler is executed, but only the storeSchedule() of the picked one will be executed.
     * The schedule() simply call the sequence of execSchedule() and storeSchedule().
     */
    virtual void schedule() override;

    /// calls LteSchedulerEnbUl::racschedule()
    virtual bool scheduleRacRequests() override;

    virtual bool scheduleRetransmissions() override;

    /*
     * prepare the set of active connections on this carrier
     * used by scheduling modules
     */
    virtual void buildCarrierActiveConnectionSet();

};

#endif // _GNB_FDSCHEME_H_

