//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// a single layer of NRSchedulerGnbUl class in simu5g
// simulate the MAC stack of the NIC module of gNB
// LteSchedulerEnb --> LteSchedulerEnbUl --> NRSchedulerGnbUl
//

#ifndef _MECRT_GNB_SCHEDULER_UL_H_
#define _MECRT_GNB_SCHEDULER_UL_H_

#include "mecrt/nic/mac/GnbMac.h"
#include "stack/mac/scheduler/NRSchedulerGnbUl.h"
#include "mecrt/nic/mac/allocator/GnbAllocationModule.h"

/**
 * @class GnbSchedulerUl
 *
 * 5G gNB uplink scheduler.
 * Note that this is not a OMNET module.
 */
class GnbSchedulerUl : public NRSchedulerGnbUl
{
    // XXX debug: to call grant from mac
    friend class LteMacEnb;
    friend class GnbMac;

    friend class LteScheduler;
    friend class LteDrr;
    friend class LtePf;
    friend class LteMaxCi;
    friend class LteMaxCiMultiband;
    friend class LteMaxCiOptMB;
    friend class LteMaxCiComp;
    friend class LteAllocatorBestFit;
    friend class FDSchemeUl;

  public:

    // ================================
    // ====== LteSchedulerEnbUl =======
    // ================================

    /**
     * Updates current schedule list with RAC grant responses.
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool racschedule(double carrierFrequency, BandLimitVector* bandLim = NULL) override;
    virtual void racscheduleBackground(unsigned int& racAllocatedBlocks, double carrierFrequency, BandLimitVector* bandLim = NULL) override;

    /**
     * Updates current schedule list with HARQ retransmissions.
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxschedule(double carrierFrequency, BandLimitVector* bandLim = NULL) override;

    /**
     * Schedule retransmissions for background UEs
     * @return TRUE if OFDM space is exhausted.
     */
    virtual bool rtxscheduleBackground(double carrierFrequency, BandLimitVector* bandLim = NULL) override;

    /**
     * Schedules retransmission for the Harq Process of the given UE on a set of logical bands.
     * Each band has also assigned a band limit amount of bytes: no more than the specified
     * amount will be served on the given band for the acid.
     *
     * @param nodeId The node ID
     * @param carrierFrequency carrier frequency
     * @param cw The codeword used to serve the acid process
     * @param bands A vector of logical bands
     * @param acid The ACID
     * @return The allocated bytes. 0 if retransmission was not possible
     */
    virtual unsigned int schedulePerAcidRtx(MacNodeId nodeId, double carrierFrequency, Codeword cw, unsigned char acid,
        std::vector<BandLimit>* bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false) override;

    virtual unsigned int schedulePerAcidRtxD2D(MacNodeId destId, MacNodeId senderId, double carrierFrequency, Codeword cw, unsigned char acid,
        std::vector<BandLimit>* bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false);

    virtual unsigned int scheduleBgRtx(MacNodeId bgUeId, double carrierFrequency, Codeword cw, std::vector<BandLimit>* bandLim = nullptr,
            Remote antenna = MACRO, bool limitBl = false) override;

    /**
     * Returns the available space for a given (background) user, antenna, logical band and codeword, in bytes.
     *
     * @param id MAC node Id
     * @param antenna antenna
     * @param b band
     * @param cw codeword
     * @return available space in bytes
     */
    virtual unsigned int availableBytesBackgroundUe(const MacNodeId id, const Remote antenna, Band b, Direction dir, double carrierFrequency, int limit = -1);


    // does nothing with asynchronous H-ARQ
    virtual void updateHarqDescs() override { }

  protected:

    // System allocator, carries out the block-allocation functions.
    GnbAllocationModule *allocator_;

    /***
     * number of resource blocks per band
     */
    unsigned int rbPerBand_;

    // ================================
    // ======= LteSchedulerEnb ========
    // ================================

    /**
     * Set Direction and bind the internal pointers to the MAC objects.
     * @param dir link direction
     * @param mac pointer to MAC module
     */
    virtual void initialize(Direction dir, LteMacEnb* mac);

    /**
     * Schedule data. Returns one schedule list per carrier
     * @param list
     */
    virtual std::map<double, LteMacScheduleList>* schedule() override;

    /**
     * Adds an entry (if not already in) to scheduling list.
     * The function calls the LteScheduler notify().
     * @param cid connection identifier
     */
    virtual void backlog(MacCid cid);

    /**
     * Schedules capacity for a given connection without effectively perform the operation on the
     * real downlink/uplink buffer: instead, it performs the operation on a virtual buffer,
     * which is used during the finalize() operation in order to commit the decision performed
     * by the grant function.
     * Each band has also assigned a band limit amount of bytes: no more than the
     * specified amount will be served on the given band for the cid
     *
     * @param cid Identifier of the connection that is granted capacity
     * @param antenna the DAS remote antenna
     * @param bands The set of logical bands with their related limits
     * @param bytes Grant size, in bytes
     * @param terminate Set to true if scheduling has to stop now
     * @param active Set to false if the current queue becomes inactive
     * @param eligible Set to false if the current queue becomes ineligible
     * @param limitBl if true bandLim vector express the limit of allocation for each band in block
     * @return The number of bytes that have been actually granted.
     */
    virtual unsigned int scheduleGrant(MacCid cid, unsigned int bytes, bool& terminate, bool& active, bool& eligible, double carrierFrequency,
            BandLimitVector* bandLim = nullptr, Remote antenna = MACRO, bool limitBl = false) override;

    /**
     * Returns the number of available blocks for the UE on the given antenna
     * in the logical band provided.
     */
    virtual unsigned int readAvailableRbs(const MacNodeId id, const Remote antenna, const Band b);

    /**
     * Returns the available space for a given user, antenna, logical band and codeword, in bytes.
     *
     * @param id MAC node Id
     * @param antenna antenna
     * @param b band
     * @param cw codeword
     * @return available space in bytes
     */
    unsigned int availableBytes(const MacNodeId id, const Remote antenna, Band b, Codeword cw, Direction dir, double carrierFrequency, int limit = -1);


    /*****************
     * UTILITIES
     *****************/

    /***
     * search the SchedDisciplineTable defined in LteCommon.h; currently, 7 disciplines has been defined.
     * if define more disciplines, need to modify the table as well
     * @param name the schedule discipline name
     */
    virtual SchedDiscipline getSchedDiscipline(std::string name);

    /**
     * Returns a particular LteScheduler subclass,
     * implementing the given discipline.
     * @param discipline scheduler discipline
     */
    virtual LteScheduler* getScheduler(SchedDiscipline discipline);
    virtual LteScheduler* getScheduler(SchedDiscipline discipline, std::string discipline_name);

};

#endif // _LTE_LTE_SCHEDULER_ENB_DL_H_
