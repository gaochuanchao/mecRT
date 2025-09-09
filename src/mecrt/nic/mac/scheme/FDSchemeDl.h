//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of LteMaxCi class in simu5g
// LteScheduler --> LteMaxCi
//

#ifndef _GNB_FDSCHEMEDL_H_
#define _GNB_FDSCHEMEDL_H_

#include "stack/mac/scheduler/LteScheduler.h"
#include "mecrt/nic/mac/scheduler/GnbSchedulerDl.h"

class FDSchemeDl : public virtual LteScheduler
{
  protected:

    typedef SortedDesc<MacCid, unsigned int> ScoreDesc;
    typedef std::priority_queue<ScoreDesc> ScoreList;

    /// Associated GnbSchedulerDl (it is the one who creates the FDSchemeDl)
    GnbSchedulerDl* eNbScheduler_;

  public:

    virtual void prepareSchedule() override;

    virtual void commitSchedule() override;

    /**
     * Initializes the GnbSchedulerDl.
     * @param gNbSchedulerDl gNb downlink scheduler
     */
    virtual void setGnbSchedulerDl(GnbSchedulerDl* gNbSchedulerDl) { eNbScheduler_ =  gNbSchedulerDl;}

    /// performs request of grant to the eNbScheduler
    virtual unsigned int requestGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible , std::vector<BandLimit>* bandLim = nullptr) override;


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

    virtual bool scheduleRacRequests() override;

    virtual bool scheduleRetransmissions() override;

    /*
     * prepare the set of active connections on this carrier
     * used by scheduling modules
     */
    virtual void buildCarrierActiveConnectionSet();

};

#endif // _GNB_FDSCHEME_H_

