//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    MecCommon.ned
//
//  Description:
//    Collect metadata for different submodules of this node,
//    make it easier to fetch the information from different modules within the same node.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-16
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

package mecRT.mecrt.common;

simple NodeInfo
{
    parameters:
        @display("i=block/control");

        string nodeType; // GNODEB, UE
        // double ifFailProb = default(0.0); // probability of interface failure per second
        // double nodeFailProb = default(0.0); // probability of node failure per second
        double ifFailTime = default(0.0); // time when the interface is down
        double ifRecoverTime = default(0.0); // time when the interface is up
        double nodeFailTime = default(0.0); // time when the node is down
        double nodeRecoverTime = default(0.0); // time when the node is up

        @signal[linkStateChanged](type="double");   // record the time when link state changed
        @statistic[linkStateChanged](title="link state changed time"; source="linkStateChanged"; record=vector);
}
