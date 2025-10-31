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

package mecRT.mecrt.apps.server;

import inet.applications.contract.IApp;

simple Server like IApp
{
    parameters:
        int localPort = default(-1);
        int cmpUnitTotal = default(16);
        string resourceType = default("GPU");	// whether using GPU or CPU
        int serviceInitMinTime = default(10);	// the minimum service starting time
        int serviceInitMaxTime = default(50);	// the maximum service starting time
        string nodeInfoModulePath = default("^.nodeInfo");
        
        @signal[meetDlPkt](type="int");
        @statistic[meetDlPkt](title="number of jobs meeting deadline"; source="meetDlPkt"; record=sum);

        @signal[failedSrvDownPkt](type="int");
        @statistic[failedSrvDownPkt](title="number of jobs failed due to server down"; source="failedSrvDownPkt"; record=sum);

        @signal[missDlPkt](type="int");
        @statistic[missDlPkt](title="number of jobs missing deadline"; source="missDlPkt"; record=sum);

        @display("i=block/app2");
    gates:
        output socketOut;
        input socketIn;
}
