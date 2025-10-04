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

package mecRT.mecrt.nic.ip2nic;

import simu5g.stack.ip2nic.INetworkLayer2CellularNic;

//
// IP2Nic module performs as a bridge between the IP network layer and the LTE/NR protocol stack
// It has a slightly different behavior in the UE rather than in the BS
// It basically:
// - Registers itself as an interface for being accessible from upper layers (e.g. IP Layer)
// - In the downstream direction adds control information to IP packets (5-tuple). Moreover in the BS 
//   module it adds the destination id to the control information, in order to send packets to the correct UE  
//
simple MecIP2Nic like INetworkLayer2CellularNic
{
    parameters:
        @class("MecIP2Nic");
        string nodeType;    
        string interfaceTableModule;
        string interfaceName = default("cellular"); 
        string routingTableModule;
        string nodeInfoModulePath = default("^.^.nodeInfo");
        int mtu @unit(B) = default(1500B); // Maximum Transmission Unit
        @display("i=block/layer");
    gates:
        // connection to network layer.
        // These gate names are recognized by the network layer (same as the ones in the INetworkInterface interface) 
        input upperLayerIn;
        output upperLayerOut;

        // connection to the cellular stack
        inout stackNic @labels(IPDatagram);
}
