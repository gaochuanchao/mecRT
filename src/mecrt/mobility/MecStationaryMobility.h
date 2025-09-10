//
// Copyright (C) 2006 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//

#ifndef __MECRT_STATIONARYMOBILITY_H
#define __MECRT_STATIONARYMOBILITY_H

#include "inet/mobility/base/StationaryMobilityBase.h"
#include "mecrt/common/Database.h"

using namespace inet;

/**
 * This mobility module does not move at all; it can be used for standalone stationary nodes.
 *
 * @ingroup mobility
 */
class MecStationaryMobility : public StationaryMobilityBase
{
  protected:
    bool enableInitDebug_ = false; // whether to enable the debug info during initialization
    bool updateFromDisplayString;
    Database *database_;     // the database module
    int nodeVectorIdx_;    // the index of the node in the vector list

  protected:
    virtual void initialize(int stage) override;
    virtual void refreshDisplay() const override;
    virtual void updateMobilityStateFromDisplayString();

    /** @brief Initializes mobility position. */
    virtual void initializePosition() override;

    /** @brief Initializes the position from the display string or from module parameters. */
    virtual void setInitialPosition() override;
};

#endif