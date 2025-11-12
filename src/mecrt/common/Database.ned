//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

package mecRT.mecrt.common;

// store all the necessary data for VEC experiments
simple Database 
{
    parameters:
        // store the application local execution profiling data
        string ueExeDataPath = default("");

        // store the gNB execution profiling data
        string gnbExeDataPath = default("");

        // store the application data size
        string appDataSizePath = default("");

        // store the gNB location
        string gnbPosDataPath = default("");

        // the idle power of vehicle computing device, default 1900mW
        double idlePower = default(1900);
        
        // offloading power of vehicle NIC, default 2210mW
        double offloadPower = default(2210);

        // gNB execution scale, default 1.0
        double gnbExeScale = default(1.0);

        // parameters to inject errors
        int numLinks = default(0); // number of links in the network
        bool linkErrorInjection = default(false);
        double linkErrorProb = default(0.0);
        bool serverErrorInjection = default(false);
        double serverErrorProb = default(0.0);
        double failureRecoveryInterval @unit(s) = default(30s); // time for failure recovery
        bool routeUpdate = default(true); // whether to update routes after failures

        @signal[databaseGrantedAppUtility](type="double");
        @signal[databaseGrantedAppCount](type="int");
        @statistic[databaseGrantedAppUtility](title="granted app utility collected by database"; source="databaseGrantedAppUtility"; record=vector, mean, sum);
        @statistic[databaseGrantedAppCount](title="granted app count collected by database"; source="databaseGrantedAppCount"; record=vector, mean, sum);

        @display("i=block/cogwheel");
}
