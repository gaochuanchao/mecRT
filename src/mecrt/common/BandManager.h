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
#ifndef _VEC_BANDMANAGER_H_
#define _VEC_BANDMANAGER_H_

#include <string.h>
#include <omnetpp.h>
#include <inet/common/INETDefs.h>
#include "common/LteCommon.h"
#include "common/binder/Binder.h"

#include "mecrt/nic/phy/UePhy.h"

using namespace omnetpp;
using namespace std;

class UePhy;

class BandManager : public omnetpp::cSimpleModule
{

  protected:
	bool enableInitDebug_;
    Binder* binder_;
	Remote antenna_;
	Direction dir_;
	double ttiPeriod_;
	double frequency_;

	omnetpp::cMessage* updateTick_;

	// {ueId: {destId: {band: endTime}}}, the map of the transmission parameters
	map<MacNodeId, map<MacNodeId, map<Band, simtime_t>>> transmitMapUl_;
	
	map<MacNodeId, UePhy*> uePhy_;
	map<MacNodeId, double> offloadPower_;

	// =========== define signals ===========
	simsignal_t offloadEnergyConsumedSignal_;  // signal for offload energy consumed
	double offloadConsumedEnergy_;  // the energy consumed for offloading at TTI

  public:
    BandManager();
    ~BandManager();

    virtual void initialize(int stage) override;
	virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

	virtual void handleMessage(omnetpp::cMessage *msg) override;

	virtual void addUePhy(MacNodeId ueId, UePhy* phy, double power) 
	{ 
		uePhy_[ueId] = phy;
		offloadPower_[ueId] = power;
	}

	virtual void addTransmissionUl(MacNodeId ueId, MacNodeId destId, RbMap& rbMap, simtime_t endTime);

	virtual void updateTransmissionUl();
};

#endif
