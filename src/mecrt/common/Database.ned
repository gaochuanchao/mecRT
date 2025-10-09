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
        // store the application local execution time, execution power
        string vehExeDataPath = default("");
        
        // store the server execution time
        string rsuExeDataPath = default("");
        
        // store the RSU/gNB location
        string rsuPosDataPath = default("");

        // store the RSU graph data
        string rsuGraphPath = default("");

        // the idle power of vehicle computing device, default 1900mW
        double idlePower = default(1900);
        
        // offloading power of vehicle NIC, default 2210mW
        double offloadPower = default(2210);

        // server execution scale, default 1.0
        double serverExeScale = default(1.0);

        @display("i=block/cogwheel");
}
