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

package mecRT.mecrt.coreNetwork;

//
// Implements a simple routing-to-user module
// when data is routing in the backhaul network, it is sent to this module of a gnb
// if the destination is the user device
//
simple MecRtUser
{
    parameters:
        string interfaceTableModule;            // path to the InterfaceTable module
        
        @display("i=block/tunnel");

    gates:
        output socketOut;
        input socketIn;

        output pppGate;
}
