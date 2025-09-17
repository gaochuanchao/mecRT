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

// define the backhaul network topology
simple NetTopology 
{
    parameters:
        // the path of the topology file
        string topologyFile = default("");

        // the link type to connect nodes
        string linkType = default("inet.node.ethernet.Eth100G");

        // display offset between gnb and gnbUpf
        int displayOffsetX = default(30);
        int displayOffsetY = default(20);

        @display("i=block/cogwheel");
}
