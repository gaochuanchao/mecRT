//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecDlFeedbackGenerator.cc / MecDlFeedbackGenerator.h
//
//  Description:
//    This file implements the downlink feedback generator functionality in the MEC system.
//    The UE sends the souding reference signals (SRS) to the RSU periodically for RSU to estimate
//    the uplink channel quality. The RSU then decides the uplink resource allocation and feedbacks to the UE.
//
//  Original code from Simu5G (https://github.com/Unipisa/Simu5G/blob/master/src/stack/phy/feedback/LteDlFeedbackGenerator.h)
//
//  Adjustment:
//    - Add periodic feedback generation function.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
#include "mecrt/feedback/MecDlFeedbackGenerator.h"
#include "stack/phy/layer/LtePhyUe.h"

Define_Module(MecDlFeedbackGenerator);

using namespace omnetpp;
using namespace inet;

/******************************
 *    PROTECTED FUNCTIONS
 ******************************/

void MecDlFeedbackGenerator::initialize(int stage)
{
    EV << "DlFeedbackGenerator stage: " << stage << endl;
    if (stage == INITSTAGE_LOCAL)
    {
        // Read NED parameters
        fbPeriod_ = (simtime_t)(int(par("fbPeriod")) * TTI);// TTI -> seconds
        fbDelay_ = (simtime_t)(int(par("fbDelay")) * TTI);// TTI -> seconds
        if (fbPeriod_ <= fbDelay_)
        {
            error("Feedback Period MUST be greater than Feedback Delay");
        }
        fbType_ = getFeedbackType(par("feedbackType").stringValue());
        rbAllocationType_ = getRbAllocationType(
            par("rbAllocationType").stringValue());
        usePeriodic_ = par("usePeriodic");
        currentTxMode_ = aToTxMode(par("initialTxMode"));

        generatorType_ = getFeedbackGeneratorType(
            par("feedbackGeneratorType").stringValue());

        // TODO: find a more elegant way
        if (strcmp(getFullName(), "nrDlFbGen") == 0)
            masterId_ = getAncestorPar("nrMasterId");
        else
            masterId_ = getAncestorPar("masterId");
        nodeId_ = getAncestorPar("macNodeId");

        /** Initialize timers **/

        tPeriodicSensing_ = new TTimer(this);
        tPeriodicSensing_->setTimerId(PERIODIC_SENSING);

        tPeriodicTx_ = new TTimer(this);
        tPeriodicTx_->setTimerId(PERIODIC_TX);

        tAperiodicTx_ = new TTimer(this);
        tAperiodicTx_->setTimerId(APERIODIC_TX);
        feedbackComputationPisa_ = false;
        WATCH(fbType_);
        WATCH(rbAllocationType_);
        WATCH(fbPeriod_);
        WATCH(fbDelay_);
        WATCH(usePeriodic_);
        WATCH(currentTxMode_);
    }
    else if (stage == INITSTAGE_LINK_LAYER)
    {
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " init" << endl;

        if (masterId_ > 0)  // only if not detached
            initCellInfo();

        LtePhyUe* tmp;
        // TODO: find a more elegant way
        if (strcmp(getFullName(), "nrDlFbGen") == 0)
            tmp = dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("nrPhy"));
        else
            tmp = dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("phy"));

        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " phyUe taken" << endl;
        dasFilter_ = tmp->getDasFilter();
        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " phyUe used" << endl;
//        initializeFeedbackComputation(par("feedbackComputation").xmlValue());


        // TODO: remove this parameter
        feedbackComputationPisa_ = true;

        EV << "DLFeedbackGenerator Stage " << stage << " nodeid: " << nodeId_
           << " feedback computation initialize" << endl;
        WATCH(numBands_);
        WATCH(numPreferredBands_);

		mobility_ = check_and_cast<MecMobility*>(getParentModule()->getParentModule()->getSubmodule("mobility"));
        moveStartTime_ = mobility_->getMoveStartTime();
        moveStoptime_ = mobility_->getMoveStopTime();

        if (masterId_ > 0 && usePeriodic_)
        {
            int num = intuniform(1, int(par("fbPeriod")));
            tPeriodicSensing_->start(moveStartTime_ + num * TTI);
        }
    }
	
}

void MecDlFeedbackGenerator::handleMessage(cMessage *msg)
{
    TTimerMsg *tmsg = check_and_cast<TTimerMsg*>(msg);
    FbTimerType type = (FbTimerType) tmsg->getTimerId();

	if (simTime() >= moveStoptime_)
	{
		EV << "VecApp::handleMessage - stop dlFeedback for node " << nodeId_ << "!" <<endl;
		tPeriodicSensing_->stop();
		tPeriodicTx_->stop();
		tAperiodicTx_->stop();
		return;
	}

    if (type == PERIODIC_SENSING)
    {
        EV << NOW << " Periodic Sensing" << endl;
        tPeriodicSensing_->handle();
        tPeriodicSensing_->start(fbPeriod_);
        // sensing(PERIODIC);
        sendFeedback(periodicFeedback, PERIODIC);   // directly send feedback, omit the sensing delay
    }
    else if (type == PERIODIC_TX)
    {
        EV << NOW << " Periodic Tx" << endl;
        tPeriodicTx_->handle();
        sendFeedback(periodicFeedback, PERIODIC);
    }
    else if (type == APERIODIC_TX)
    {
        EV << NOW << " Aperiodic Tx" << endl;
        tAperiodicTx_->handle();
        sendFeedback(aperiodicFeedback, APERIODIC);
    }
    delete tmsg;
}

void MecDlFeedbackGenerator::initCellInfo()
{
    cellInfo_ = getCellInfo(masterId_);
    EV << "DLFeedbackGenerator - nodeid: " << nodeId_ << " cellInfo taken" << endl;

    if (cellInfo_ != NULL)
    {
        antennaCws_ = cellInfo_->getAntennaCws();
        numBands_ = cellInfo_->getPrimaryCarrierNumBands();
        numPreferredBands_ = cellInfo_->getNumPreferredBands();
    }
    else
        throw cRuntimeError("MecDlFeedbackGenerator::initCellInfo - cellInfo is NULL pointer. Aborting");

    EV << "DLFeedbackGenerator - nodeid: " << nodeId_
       << " used cellInfo: bands " << numBands_ << " preferred bands "
       << numPreferredBands_ << endl;
}


void MecDlFeedbackGenerator::sensing(FbPeriodicity per)
{
    if (per == PERIODIC && tAperiodicTx_->busy()
        && tAperiodicTx_->elapsed() < 0.001)
    {
        /* In this TTI an APERIODIC sensing has been done
         * (an APERIODIC tx is scheduled).
         * Ignore this PERIODIC request.
         */
        EV << NOW << " Aperiodic before Periodic in the same TTI: ignore Periodic" << endl;
        return;
    }

    if (per == APERIODIC && tAperiodicTx_->busy())
    {
        /* An APERIODIC tx is already scheduled:
         * ignore this request.
         */
        EV << NOW << " Aperiodic overlapping: ignore second Aperiodic" << endl;
        return;
    }

    if (per == APERIODIC && tPeriodicTx_->busy()
        && tPeriodicTx_->elapsed() < 0.001)
    {
        /* In this TTI a PERIODIC sensing has been done.
         * Deschedule the PERIODIC tx and continue with APERIODIC.
         */
        EV << NOW << " Periodic before Aperiodic in the same TTI: remove Periodic" << endl;
        tPeriodicTx_->stop();
    }

    // Schedule feedback transmission
    if (per == PERIODIC)
        tPeriodicTx_->start(fbDelay_);
    else if (per == APERIODIC)
        tAperiodicTx_->start(fbDelay_);
}

        /***************************
         *    PUBLIC FUNCTIONS
         ***************************/

MecDlFeedbackGenerator::MecDlFeedbackGenerator()
{
    tPeriodicSensing_ = nullptr;
    tPeriodicTx_ = nullptr;
    tAperiodicTx_ = nullptr;
}

MecDlFeedbackGenerator::~MecDlFeedbackGenerator()
{
    if (tPeriodicSensing_ != nullptr)
        delete tPeriodicSensing_;
    
    if (tPeriodicTx_ != nullptr)
        delete tPeriodicTx_;

    if (tAperiodicTx_ != nullptr)
        delete tAperiodicTx_;
}

void MecDlFeedbackGenerator::aperiodicRequest()
{
    Enter_Method("aperiodicRequest()");
    EV << NOW << " Aperiodic request" << endl;
    sensing(APERIODIC);
}

void MecDlFeedbackGenerator::setTxMode(TxMode newTxMode)
{
    Enter_Method("setTxMode()");
    currentTxMode_ = newTxMode;
}

void MecDlFeedbackGenerator::sendFeedback(LteFeedbackDoubleVector fb,
    FbPeriodicity per)
{
    EV << "sendFeedback() in DL" << endl;
    EV << "Periodicity: " << periodicityToA(per) << " nodeId: " << nodeId_ << endl;

    FeedbackRequest feedbackReq;
    if (feedbackComputationPisa_)
    {
        feedbackReq.request = true;
        feedbackReq.genType = getFeedbackGeneratorType(
            getAncestorPar("feedbackGeneratorType").stringValue());
        feedbackReq.type = getFeedbackType(par("feedbackType").stringValue());
        feedbackReq.txMode = currentTxMode_;
        feedbackReq.rbAllocationType = rbAllocationType_;
    }
    else
    {
        feedbackReq.request = false;
    }

    //use PHY function to send feedback
    // TODO: find a more elegant way
    if (strcmp(getFullName(), "nrDlFbGen") == 0)
        (dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("nrPhy")))->sendFeedback(fb, fb, feedbackReq);
    else
        (dynamic_cast<LtePhyUe*>(getParentModule()->getSubmodule("phy")))->sendFeedback(fb, fb, feedbackReq);
}

// TODO adjust default value
LteFeedbackComputation* MecDlFeedbackGenerator::getFeedbackComputationFromName(
    std::string name, ParameterMap& params)
{
    ParameterMap::iterator it;
    if (name == "REAL")
    {
        feedbackComputationPisa_ = true;
        return 0;
    }
    else
        return 0;
}


void MecDlFeedbackGenerator::handleHandover(MacCellId newEnbId)
{
    Enter_Method("MecDlFeedbackGenerator::handleHandover()");
    masterId_ = newEnbId;
    if (masterId_ != 0)
    {
        initCellInfo();
        EV << NOW << " MecDlFeedbackGenerator::handleHandover - Master ID updated to " << masterId_ << endl;
        if (tPeriodicSensing_->idle())  // resume feedback
            tPeriodicSensing_->start(0);
    }
    else
    {
        cellInfo_ = NULL;

        // stop measuring feedback
        tPeriodicSensing_->stop();
    }
}
