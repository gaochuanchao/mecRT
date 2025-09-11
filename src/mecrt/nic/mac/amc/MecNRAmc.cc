//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecNRAmc.cc / MecNRAmc.h
//
//  Description:
//    This file implements the NR AMC module for the MEC environment.
//    Original file: simu5g - "stack/mac/amc/NRAmc.h"
//    The original AMC module is designed for 1 RB per band, which is not efficient for SRS feedback.
//    We update the AMC module such that one band can contains multiple RBs, allowing users to decide the
//    the bandwidth resource granularity more flexibly.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
#include "mecrt/nic/mac/amc/MecNRAmc.h"
#include "mecrt/nic/mac/amc/MecPilot.h"

using namespace std;
using namespace omnetpp;

MecNRAmc::MecNRAmc(LteMacEnb *mac, Binder *binder, CellInfo *cellInfo, int numAntennas)
           : NRAmc(mac, binder, cellInfo, numAntennas)
{
    EV << "Reset Amc pilot to VecPilot"<< endl;
    delete pilot_;
    pilot_ = nullptr;
    pilot_ = new MecPilot(this);
}

MecNRAmc::~MecNRAmc()
{
}

unsigned int MecNRAmc::getSymbolsPerSlot(double carrierFrequency, Direction dir)
{
    unsigned totSymbols = 14;   // TODO get this parameter from CellInfo/Carrier

    // use a function from the binder
    SlotFormat sf = binder_->getSlotFormat(carrierFrequency);
    if (!sf.tdd)
        return totSymbols;

    // TODO handle FLEX symbols: so far, they are used as guard (hence, not used for scheduling)
    if (dir == DL)
        return sf.numDlSymbols;
    // else UL
    return sf.numUlSymbols;
}

unsigned int MecNRAmc::getResourceElementsPerBlock(unsigned int symbolsPerSlot)
{
    unsigned int numSubcarriers = 12;   // TODO get this parameter from CellInfo/Carrier
    unsigned int reSignal = 1;
    unsigned int nOverhead = 0;

    if (symbolsPerSlot == 0)
        return 0;
    return (numSubcarriers * symbolsPerSlot) - reSignal - nOverhead;
}

unsigned int MecNRAmc::getResourceElements(unsigned int blocks, unsigned int symbolsPerSlot)
{
    unsigned int numRePerBlock = getResourceElementsPerBlock(symbolsPerSlot);

    if (numRePerBlock > 156)
        return 156 * blocks;

    return numRePerBlock * blocks;
}

unsigned int MecNRAmc::computeTbsFromNinfo(double nInfo, double coderate)
{
    unsigned int tbs = 0;
    unsigned int _nInfo = 0;
    unsigned int n = 0;
    if (nInfo == 0)
        return 0;

    if (nInfo <= 3824)
    {
        n = std::max((int)3, (int)(floor(log2(nInfo) - 6)));
        _nInfo = std::max((unsigned int)24, (unsigned int)((1 << n) * floor(nInfo/(1<<n))) );

        // get tbs from table
        unsigned int j = 0;
        for (j = 0; j < TBSTABLESIZE-1; j++)
        {
            if (nInfoToTbs[j] >= _nInfo )
                break;
        }

        tbs = nInfoToTbs[j];
    }
    else
    {
        unsigned int C;
        n = floor( log2(nInfo - 24) - 5);
        _nInfo = ( 1 << n ) * round( (nInfo - 24) / (1 << n));
        if (coderate <= 0.25 )
        {
            C = ceil( (_nInfo+24) / 3816 );
            tbs = 8 * C * ceil( (_nInfo+24) / (8*C) ) - 24;
        }
        else
        {
            if (_nInfo >= 8424)
            {
                C = ceil( (_nInfo+24) / 8424 );
                tbs = 8 * C * ceil( (_nInfo+24) / (8*C) ) - 24;
            }
            else
            {
                tbs = 8 * ceil( (_nInfo+24) / 8 ) - 24;
            }
        }
    }
    return tbs;
}

unsigned int MecNRAmc::computeCodewordTbs(UserTxParams* info, Codeword cw, Direction dir, unsigned int numRe)
{
    std::vector<unsigned char> layers = info->getLayers();
    NRMCSelem mcsElem = getMcsElemPerCqi(info->readCqiVector().at(cw), dir);
    unsigned int modFactor;
    switch(mcsElem.mod_)
    {
        case _QPSK:   modFactor = 2; break;
        case _16QAM:  modFactor = 4; break;
        case _64QAM:  modFactor = 6; break;
        case _256QAM: modFactor = 8; break;
        default: throw cRuntimeError("MecNRAmc::computeCodewordTbs - unrecognized modulation.");
    }
    double coderate = mcsElem.coderate_ / 1024;
    double nInfo = numRe * coderate * modFactor * layers.at(cw);

    return computeTbsFromNinfo(floor(nInfo),coderate);
}

/********************
 * PUBLIC FUNCTIONS
 ********************/


const UserTxParams& MecNRAmc::computeTxParams(MacNodeId id, const Direction dir, double carrierFrequency)
{
    // DEBUG
    EV << NOW << " MecNRAmc::computeTxParams --------------::[ START ]::--------------\n";
    EV << NOW << " MecNRAmc::computeTxParams CellId: " << cellId_ << "\n";
    EV << NOW << " MecNRAmc::computeTxParams NodeId: " << id << "\n";
    EV << NOW << " MecNRAmc::computeTxParams Direction: " << dirToA(dir) << "\n";
    EV << NOW << " MecNRAmc::computeTxParams - - - - - - - - - - - - - - - - - - - - -\n";
    EV << NOW << " MecNRAmc::computeTxParams RB allocation type: " << allocationTypeToA(allocationType_) << "\n";
    EV << NOW << " MecNRAmc::computeTxParams - - - - - - - - - - - - - - - - - - - - -\n";

    const UserTxParams &info = pilot_->computeTxParams(id,dir,carrierFrequency);
    EV << NOW << " MecNRAmc::computeTxParams --------------::[  END  ]::--------------\n";

    return info;
}

unsigned int MecNRAmc::computeBytesOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    EV << NOW << " MecNRAmc::blocks2bytes Node " << id << ", Band " << b << ", direction " << dirToA(dir) << ", blocks " << blocks << "\n";

    unsigned int bits = computeBitsOnNRbs(id, b, blocks, dir, carrierFrequency);
    unsigned int bytes = bits/8;

    // DEBUG
    EV << NOW << " MecNRAmc::blocks2bytes Resource Blocks: " << blocks << "\n";
    EV << NOW << " MecNRAmc::blocks2bytes Available space: " << bits << "\n";
    EV << NOW << " MecNRAmc::blocks2bytes Available space: " << bytes << "\n";

    return bytes;
}

unsigned int MecNRAmc::computeBytesOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    EV << NOW << " MecNRAmc::blocks2bytes Node " << id << ", Band " << b << ", Codeword " << cw << ",  direction " << dirToA(dir) << ", blocks " << blocks << "\n";

    unsigned int bits = computeBitsOnNRbs(id, b, cw, blocks, dir, carrierFrequency);
    unsigned int bytes = bits/8;

    // DEBUG
    EV << NOW << " MecNRAmc::blocks2bytes Resource Blocks: " << blocks << "\n";
    EV << NOW << " MecNRAmc::blocks2bytes Available space: " << bits << "\n";
    EV << NOW << " MecNRAmc::blocks2bytes Available space: " << bytes << "\n";

    return bytes;
}


unsigned int MecNRAmc::computeBitsOnNRbs(MacNodeId id, Band b, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Node: " << id << "\n";
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Band: " << b << "\n";
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Direction: " << dirToA(dir) << "\n";

    unsigned int numRe = getResourceElements(blocks, getSymbolsPerSlot(carrierFrequency, dir));

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir,carrierFrequency);

    unsigned int bits = 0;
    unsigned int codewords = info.getLayers().size();
    for (Codeword cw = 0; cw < codewords; ++cw)
    {
        // if CQI == 0 the UE is out of range, thus bits=0
        if (info.readCqiVector().at(cw) == 0)
        {
            EV << NOW << " MecNRAmc::computeBitsOnNRbs - CQI equal to zero on cw " << cw << ", return no blocks available" << endl;
            continue;
        }

        unsigned int tbs = computeCodewordTbs(&info, cw, dir, numRe);
        bits += tbs;
    }

    // DEBUG
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Resource Blocks: " << blocks << "\n";
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Available space: " << bits << "\n";

    return bits;
}

unsigned int MecNRAmc::computeBitsOnNRbs(MacNodeId id, Band b, Codeword cw, unsigned int blocks, const Direction dir, double carrierFrequency)
{
    if (blocks == 0)
        return 0;

    // DEBUG
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Node: " << id << "\n";
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Band: " << b << "\n";
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Codeword: " << cw << "\n";
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Direction: " << dirToA(dir) << "\n";

    unsigned int numRe = getResourceElements(blocks, getSymbolsPerSlot(carrierFrequency, dir));

    // Acquiring current user scheduling information
    UserTxParams info = computeTxParams(id, dir,carrierFrequency);

    // if CQI == 0 the UE is out of range, thus return 0
    if (info.readCqiVector().at(cw) == 0)
    {
        EV << NOW << " MecNRAmc::computeBitsOnNRbs - CQI equal to zero, return no blocks available" << endl;
        return 0;
    }

    unsigned int tbs = computeCodewordTbs(&info, cw, dir, numRe);

    // DEBUG
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Resource Blocks: " << blocks << "\n";
    EV << NOW << " MecNRAmc::computeBitsOnNRbs Available space: " << tbs << "\n";

    return tbs;
}

bool MecNRAmc::existTxParams(MacNodeId id, const Direction dir, double carrierFrequency)
{
    std::map<double,std::vector<UserTxParams> > *txParams = (dir == DL) ? &dlTxParams_ : (dir == UL) ? &ulTxParams_ : (dir == D2D) ? &d2dTxParams_ : throw cRuntimeError("MecNRAmc::existTxparams(): Unrecognized direction");
    if (txParams->find(carrierFrequency) == txParams->end())
        return false;

    std::map<MacNodeId, unsigned int> &nodeIndex = (dir == DL) ? dlNodeIndex_ : (dir == UL) ? ulNodeIndex_ : d2dNodeIndex_;

    return (*txParams)[carrierFrequency].at(nodeIndex.at(id)).isSet();
}

const UserTxParams& MecNRAmc::setTxParams(MacNodeId id, const Direction dir, UserTxParams& info, double carrierFrequency)
{
    info.isSet() = true;

    /**
     * NOTE: if the antenna set has not been explicitly written in UserTxParams
     * by the AMC pilot, this antennas set contains only MACRO
     * (this is done setting MACRO in UserTxParams constructor)
     */

    // DEBUG
    EV << NOW << " MecNRAmc::setTxParams DAS antenna set for user " << id << " is \t";
    for (std::set<Remote>::const_iterator it = info.readAntennaSet().begin(); it != info.readAntennaSet().end(); ++it)
    {
        EV << "[" << dasToA(*it) << "]\t";
    }
    EV << endl;

    std::map<double,std::vector<UserTxParams> > *txParams = (dir == DL) ? &dlTxParams_ : (dir == UL) ? &ulTxParams_ : (dir == D2D) ? &d2dTxParams_ : throw cRuntimeError("MecNRAmc::setTxParams(): Unrecognized direction");
    std::map<MacNodeId, unsigned int> &nodeIndex = (dir == DL) ? dlNodeIndex_ : (dir == UL) ? ulNodeIndex_ : d2dNodeIndex_;
    if (txParams->find(carrierFrequency) == txParams->end())
    {
        // Initialize user transmission parameters structures
        ConnectedUesMap &connectedUe = (dir == DL) ? dlConnectedUe_ : ulConnectedUe_;
        std::vector<UserTxParams> tmp;
        tmp.resize(connectedUe.size(), UserTxParams());
        (*txParams)[carrierFrequency] = tmp;
    }
    return (*txParams)[carrierFrequency].at(nodeIndex.at(id)) = info;
}

const UserTxParams& MecNRAmc::getTxParams(MacNodeId id, const Direction dir, double carrierFrequency)
{
    if (dir == DL)
        return dlTxParams_[carrierFrequency].at(dlNodeIndex_.at(id));
    else if (dir == UL)
        return ulTxParams_[carrierFrequency].at(ulNodeIndex_.at(id));
    else if (dir == D2D)
        return d2dTxParams_[carrierFrequency].at(d2dNodeIndex_.at(id));
    else
        throw cRuntimeError("MecNRAmc::getTxParams(): Unrecognized direction");
}

void MecNRAmc::rescaleMcs(double rePerRb, Direction dir)
{
    if (dir == DL)
    {
        dlMcsTable_.rescale(rePerRb);
    }
    else if (dir == UL)
    {
        ulMcsTable_.rescale(rePerRb);
    }
    else if (dir == D2D) {
        d2dMcsTable_.rescale(rePerRb);
    }
}


void MecNRAmc::pushFeedback(MacNodeId id, Direction dir, LteFeedback fb, double carrierFrequency)
{
    EV << "Feedback from MacNodeId " << id << " (direction " << dirToA(dir) << ")" << endl;

    History_ *history;
    std::map<MacNodeId, unsigned int> *nodeIndex;

    history = getHistory(dir, carrierFrequency);
    if(dir==DL)
    {
        nodeIndex = &dlNodeIndex_;
    }
    else if(dir==UL)
    {
        nodeIndex = &ulNodeIndex_;
    }
    else
    {
        throw cRuntimeError("MecNRAmc::pushFeedback(): Unrecognized direction");
    }

    // Put the feedback in the FBHB
    Remote antenna = fb.getAntennaId();
    TxMode txMode = fb.getTxMode();
    if (nodeIndex->find(id) == nodeIndex->end())
    {
        return;
    }
    int index = (*nodeIndex).at(id);

    EV << "ID: " << id << endl;
    EV << "index: " << index << endl;
    (*history)[antenna].at(index).at(txMode).put(fb);

    // delete the old UserTxParam for this <UE_dir_carrierFreq>, so that it will be recomputed next time it's needed
    std::map<double,std::vector<UserTxParams> > *txParams = (dir == DL) ? &dlTxParams_ : (dir == UL) ? &ulTxParams_ : throw cRuntimeError("MecNRAmc::pushFeedback(): Unrecognized direction");
    if (txParams->find(carrierFrequency) != txParams->end() && txParams->at(carrierFrequency).at(index).isSet())
        (*txParams)[carrierFrequency].at(index).restoreDefaultValues();

    // DEBUG
    EV << "Antenna: " << dasToA(antenna) << ", TxMode: " << txMode << ", Index: " << index << endl;
    EV << "RECEIVED" << endl;
    fb.print(0,id,dir,"MecNRAmc::pushFeedback");
}

void MecNRAmc::pushFeedbackD2D(MacNodeId id, LteFeedback fb, MacNodeId peerId, double carrierFrequency)
{
    EV << "Feedback from MacNodeId " << id << " (direction D2D), peerId = " << peerId << endl;

    std::map<MacNodeId, History_> *history = &d2dFeedbackHistory_[carrierFrequency];
    std::map<MacNodeId, unsigned int> *nodeIndex = &d2dNodeIndex_;

    // Put the feedback in the FBHB
    Remote antenna = fb.getAntennaId();
    TxMode txMode = fb.getTxMode();
    int index = (*nodeIndex).at(id);

    EV << "ID: " << id << endl;
    EV << "index: " << index << endl;

    if (history->find(peerId) == history->end())
    {
        // initialize new history for this peering UE
        History_ newHist;

        ConnectedUesMap::const_iterator it, et;
        it = d2dConnectedUe_.begin();
        et = d2dConnectedUe_.end();
        for (; it != et; it++)  // For all UEs (D2D)
        {
            newHist[antenna].push_back(std::vector<LteSummaryBuffer>(UL_NUM_TXMODE, LteSummaryBuffer(fbhbCapacityD2D_, MAXCW, numBands_, lb_, ub_)));
        }
        (*history)[peerId] = newHist;
    }
    (*history)[peerId][antenna].at(index).at(txMode).put(fb);

    // delete the old UserTxParam for this <UE_dir_carrierFreq>, so that it will be recomputed next time it's needed
    if (d2dTxParams_.find(carrierFrequency) != d2dTxParams_.end() && d2dTxParams_.at(carrierFrequency).at(index).isSet())
        d2dTxParams_[carrierFrequency].at(index).restoreDefaultValues();


    // DEBUG
    EV << "PeerId: " << peerId << ", Antenna: " << dasToA(antenna) << ", TxMode: " << txMode << ", Index: " << index << endl;
    EV << "RECEIVED" << endl;
    fb.print(0,id,D2D,"MecNRAmc::pushFeedbackD2D");
}


const LteSummaryFeedback& MecNRAmc::getFeedback(MacNodeId id, Remote antenna, TxMode txMode, const Direction dir, double carrierFrequency)
{
    if (dir != DL && dir != UL)
        throw cRuntimeError("MecNRAmc::getFeedback(): Unrecognized direction");

    History_* history = getHistory(dir, carrierFrequency);
    std::map<MacNodeId, unsigned int>* nodeIndex = (dir == DL) ? &dlNodeIndex_ : &ulNodeIndex_;

    return (*history).at(antenna).at((*nodeIndex).at(id)).at(txMode).get();
}

const LteSummaryFeedback& MecNRAmc::getFeedbackD2D(MacNodeId id, Remote antenna, TxMode txMode, MacNodeId peerId, double carrierFrequency)
{
    if (peerId == 0)
    {
        // we returns the first feedback stored  in the structure
        std::map<MacNodeId, History_>::iterator it = d2dFeedbackHistory_.at(carrierFrequency).begin();
        for (; it != d2dFeedbackHistory_.at(carrierFrequency).end(); ++it)
        {
            if (it->first == 0) // skip fake UE 0
                continue;

            if (binder_->getD2DCapability(id, it->first))
            {
                peerId = it->first;
                break;
            }
        }

        // default feedback: when there is no feedback from peers yet (NOSIGNALCQI)
        if (peerId == 0)
            return d2dFeedbackHistory_.at(carrierFrequency).at(0).at(MACRO).at(0).at(txMode).get();
    }
    return d2dFeedbackHistory_.at(carrierFrequency).at(peerId).at(antenna).at(d2dNodeIndex_.at(id)).at(txMode).get();
}

