//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    BandManager.cc / BandManager.h
//
//  Description:
//    This file implements the bandwidth management functionality in the MEC system.
//    The BandManager is responsible for managing the radio resources and
//    scheduling the transmission of data between the user equipment (UE) and the
//    edge server (ES). This is suppose to be performed in the physical layer (PHY) of each UE.
//	  We use one BandManager module to manage the bandwidth for all UEs to reduce the simulation complexity i.e.,
//    to accelerate the simulation.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
#include "mecrt/common/BandManager.h"

Define_Module(BandManager);

BandManager::BandManager()
{
    updateTick_ = nullptr;
    enableInitDebug_ = false;
}

BandManager::~BandManager()
{
    if (updateTick_ != nullptr)
        cancelAndDelete(updateTick_);
}

void BandManager::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "BandManager::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

        antenna_ = MACRO;
        dir_ = UL;

        frequency_ = par("carrierFrequency");
        int numerology = par("numerologyIndex");
        ttiPeriod_ = getBinder()->getSlotDurationFromNumerologyIndex(numerology);
        transmitMapUl_.clear();
        uePhy_.clear();

        offloadEnergyConsumedSignal_ = registerSignal("offloadEnergyConsumed");
        offloadConsumedEnergy_ = 0.0;

        WATCH(dir_);
        WATCH(frequency_);
        WATCH(ttiPeriod_);

        if (enableInitDebug_)
            std::cout << "BandManager::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
    else if (stage == inet::INITSTAGE_LAST)
    {
        if (enableInitDebug_)
            std::cout << "BandManager::initialize - stage: INITSTAGE_LAST - begins" << std::endl;

        binder_ = getBinder();
        /***
         * Defines the scheduling priority of AirFrames.
         * AirFrames use a slightly higher priority (smaller priority value) than normal to ensure channel consistency. This means that before 
         * anything else happens at a time point t every AirFrame which ended at t has been removed and every AirFrame 
         * started at t has been added to the channel. 
         * An example where this matters is a ChannelSenseRequest which ends at the same time as an AirFrame starts (or ends). 
         * Depending on which message is handled first the result of ChannelSenseRequest would differ.
         */
        updateTick_ = new omnetpp::cMessage("updateTick");
        updateTick_->setSchedulingPriority(2);  // after the flushAppMsg in UeMac
        scheduleAt(simTime() + ttiPeriod_, updateTick_);

        if (enableInitDebug_)
            std::cout << "BandManager::initialize - stage: INITSTAGE_LAST - ends" << std::endl;
    }
}

/***
 * the ttiTick for the ue is trigger before the ttiTick for the gNB
 * meaning that the UE will update the band allocation before the gNB reset the band allocation
 */
void BandManager::handleMessage(omnetpp::cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (!strcmp(msg->getName(), "updateTick"))
        {
            // ===== reach the end of the TTI (any data generated at this TTI has reached the physical stack) =====
            // init and reset global allocation information
            binder_->initAndResetUlTransmissionInfo();

            // update the uplink band allocation for the UE
            updateTransmissionUl();

            scheduleAt(simTime() + ttiPeriod_, updateTick_);
        }
    }
}

void BandManager::addTransmissionUl(MacNodeId ueId, MacNodeId destId, RbMap& rbMap, simtime_t endTime)
{
    EV << "BandManager::addTransmissionUl - UE [" << ueId << "] - add transmission to destination [" << destId 
        << "]" << " end time " << endTime.dbl() << endl;

    // for each allocated band, store the UE info
    std::map<Band, unsigned int>::iterator it = rbMap[antenna_].begin(), et = rbMap[antenna_].end();
    for ( ; it != et; ++it)
    {
        Band b = it->first;
        if (it->second > 0)
        {
            // check if the band exist in band map
            if (transmitMapUl_[ueId][destId].find(b) == transmitMapUl_[ueId][destId].end())
            {
                transmitMapUl_[ueId][destId][b] = endTime;
            }
            else
            {
                if (transmitMapUl_[ueId][destId][b] < endTime)
                    transmitMapUl_[ueId][destId][b] = endTime;
            }
        }
    }
}

void BandManager::updateTransmissionUl()
{
    // update the uplink band allocation for the UE, {ueId: {destId: {band: endTime}}}
    set<MacNodeId> activeUes;  // set to store the active UEs
    for (auto ueIt = transmitMapUl_.begin(); ueIt != transmitMapUl_.end();)
    {
        for (auto destIt = ueIt->second.begin(); destIt != ueIt->second.end();)
        {
            RbMap rbMap;
            bool hasData = false;
            for (auto bandIt = destIt->second.begin(); bandIt != destIt->second.end();)
            {
                if (NOW < bandIt->second)   // the transmission is ongoing
                {
                    rbMap[antenna_][bandIt->first] = 1;
                    hasData = true;
                    ++bandIt;   // move to the next band
                    activeUes.insert(ueIt->first);  // add the UE to the active UEs set
                }
                else
                {
                    bandIt = destIt->second.erase(bandIt);  // remove the band from the map and move to the next band
                }
            }

            if (hasData)
            {
                MacNodeId ueId = ueIt->first;
                binder_->storeUlTransmissionMap(frequency_, antenna_, rbMap, ueId, destIt->first, uePhy_[ueId], dir_);
                ++destIt;   // move to the next destination
            }
            else
            {
                destIt = ueIt->second.erase(destIt);    // remove the destination from the map and move to the next destination
            }
        }

        if (!ueIt->second.empty())
            ++ueIt;    // move to the next UE
        else
            ueIt = transmitMapUl_.erase(ueIt);    // remove the UE from the map and move to the next UE
    }

    // update the offload energy consumed signal
    offloadConsumedEnergy_ = 0;
    for (MacNodeId ueId : activeUes)
    {
        offloadConsumedEnergy_ += offloadPower_[ueId] * ttiPeriod_;
    }

    emit(offloadEnergyConsumedSignal_, offloadConsumedEnergy_);
}

