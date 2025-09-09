//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//


#ifndef _MECRT_NRAMC_H_
#define _MECRT_NRAMC_H_

#include <omnetpp.h>
#include "stack/mac/amc/NRMcs.h"
#include "stack/mac/amc/NRAmc.h"

/**
 * @class MecNRAmc
 * @brief NR AMC module for Omnet++ simulator
 *
 * TBS determination based on 3GPP TS 38.214 v15.6.0 (June 2019)
 */
class MecNRAmc : public NRAmc
{

  protected:
    virtual unsigned int getSymbolsPerSlot(double carrierFrequency, Direction dir);
    virtual unsigned int getResourceElementsPerBlock(unsigned int symbolsPerSlot);
    virtual unsigned int getResourceElements(unsigned int blocks, unsigned int symbolsPerSlot);
    virtual unsigned int computeTbsFromNinfo(double nInfo, double coderate);

    virtual unsigned int computeCodewordTbs(UserTxParams* info, Codeword cw, Direction dir, unsigned int numRe);

  public:

    MecNRAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas);
    virtual ~MecNRAmc();

    // CodeRate MCS rescaling
    virtual void rescaleMcs(double rePerRb, Direction dir = DL);

    virtual void pushFeedback(MacNodeId id, Direction dir, LteFeedback fb, double carrierFrequency);
    virtual void pushFeedbackD2D(MacNodeId id, LteFeedback fb, MacNodeId peerId, double carrierFrequency);
    virtual const LteSummaryFeedback& getFeedback(MacNodeId id, Remote antenna, TxMode txMode, const Direction dir, double carrierFrequency);
    virtual const LteSummaryFeedback& getFeedbackD2D(MacNodeId id, Remote antenna, TxMode txMode, MacNodeId peerId, double carrierFrequency);


    virtual bool existTxParams(MacNodeId id, const Direction dir, double carrierFrequency);
    virtual const UserTxParams & getTxParams(MacNodeId id, const Direction dir, double carrierFrequency);
    virtual const UserTxParams & setTxParams(MacNodeId id, const Direction dir, UserTxParams & info, double carrierFrequency);
    virtual const UserTxParams & computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency);

    virtual unsigned int computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency) override;
    virtual unsigned int computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency) override;
    virtual unsigned int computeBytesOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency) override;
    virtual unsigned int computeBytesOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency) override;

};

#endif
