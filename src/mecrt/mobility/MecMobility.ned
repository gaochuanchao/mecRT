//
// Copyright (C) 2020 OpenSim Ltd.
//
// SPDX-License-Identifier: LGPL-3.0-or-later
//


package mecRT.mecrt.mobility;

import inet.mobility.base.MobilityBase;

//
// A mobility model that simulates a vehicle turning at defined waypoints.
//
// Waypoints are defined in a text file as x,y,z with one waypoint per line.
// When all waypoints have been reached, it loops back to the first one.
//

simple MecMobility extends MobilityBase
{
    parameters:
        @class(MecMobility);
        double maxSpeed @unit(mps) = default(30mps);
        string groundModule = default("environment.ground"); // module path of the ground module
        bool initFromDisplayString = default(true); // enables one time initialization from the subject module's display string
        int vehIndex = default(0);
        
        double updateInterval @unit(s) = default(0.05s); // the simulation time interval used to regularly signal mobility state changes and update the display
        bool faceForward = default(true);
}

