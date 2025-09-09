//
// Copyright (C) 2015 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


#ifndef __MECRT_VEHICLEMOBILITY_H
#define __MECRT_VEHICLEMOBILITY_H

#include "inet/environment/contract/IGround.h"
#include "inet/mobility/base/MobilityBase.h"

using namespace inet;
using namespace physicalenvironment;

/**
 * @brief class for moving mobility modules. Periodically emits a signal with the current mobility state.
 *
 * @ingroup mobility
 */
class MecMobility : public MobilityBase
{
  protected:
    struct Waypoint {
        double x;
        double y;
        double timestamp;

        Waypoint(double x, double y, double timestamp) : x(x), y(y), timestamp(timestamp) {}
    };

  protected:
    int vehIndex_;	// the index of the vehicle in the ini file

    simtime_t moveStartTime_;	// the start time of the provided file, start moving
    simtime_t moveStoptime_;	// the last time of provided file, stop moving

    const char *iTag_;	// the icon tag
    const char *tTag_;	// the text tag
    cMessage *showVehicle_;	// the message to show the vehicle

    // configuration
    std::vector<Waypoint> waypoints_;

    // The ground module given by the "groundModule" parameter, pointer stored for easier access.
    physicalenvironment::IGround *ground_ = nullptr;

    double maxSpeed_;
    // double heading_;
    int targetPointIndex_;

    /** @brief The message used for mobility state changes. */
    cMessage *moveTimer_;

    /** @brief The simulation time interval used to regularly signal mobility state changes.
     *
     * The 0 value turns off the signal. */
    simtime_t updateInterval_;

    /** @brief A mobility model may decide to become stationary at any time.
     *
     * The true value disables sending self messages. */
    bool stationary_;

    /** @brief The last velocity that was reported at lastUpdate. 
     * this is not the real velocity, but the increment of the position for every updateInterval_
     */
    Coord lastVelocity_;

    /** @brief The last angular velocity that was reported at lastUpdate. */
    Quaternion lastAngularVelocity_;

    /** @brief The next simulation time when the mobility module needs to update its internal state.
     *
     * The -1 value turns off sending a self message for the next mobility state change. */
    simtime_t nextChange_;	// the next change time in the provided file
	simtime_t nextUpdate_;	// the next update time based on the updateInterval_

    bool faceForward_;

  protected:

    virtual void initialize(int stage) override;

    virtual void initializePosition() override;

    virtual void handleSelfMessage(cMessage *message) override;

    /** @brief Schedules the move timer that will update the mobility state. */
    void scheduleUpdate();

    /** @brief Moves according to the mobility model to the current simulation time.
     *
     * Subclasses must override and update lastPosition, lastVelocity, lastUpdate, nextChange
     * and other state according to the mobility model.
     */
	virtual void move(bool arriveTarget);

	virtual void orient(bool arriveTarget);	

    virtual void setInitialPosition() override;

    virtual void readWaypointsFromFile(const char *fileName);

  public:
    MecMobility();
    virtual ~MecMobility();

    virtual double getMaxSpeed() const override { return maxSpeed_; }

	  virtual const Coord& getCurrentPosition() override;
    virtual const Coord& getCurrentVelocity() override;
    virtual const Coord& getCurrentAcceleration() override { throw cRuntimeError("Invalid operation"); }

    virtual const Quaternion& getCurrentAngularPosition() override;
    virtual const Quaternion& getCurrentAngularVelocity() override;
    virtual const Quaternion& getCurrentAngularAcceleration() override { throw cRuntimeError("Invalid operation"); }

    virtual simtime_t getMoveStartTime() const { return moveStartTime_; }
    virtual simtime_t getMoveStopTime() const { return moveStoptime_; }
};

#endif

