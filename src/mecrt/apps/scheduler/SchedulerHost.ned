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

package mecRT.mecrt.apps.scheduler;

import inet.common.MessageDispatcher;
import inet.linklayer.contract.IEthernetInterface;
import inet.linklayer.ppp.PppInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4NetworkLayer;
import inet.networklayer.ipv6.Ipv6NetworkLayer;
import inet.transportlayer.tcp.Tcp;
import inet.transportlayer.udp.Udp;
import mecRT.mecrt.apps.scheduler.Scheduler;
import mecRT.mecrt.routing.MecOspf;


module SchedulerHost
{
    parameters:
        @networkNode();
        @display("i=device/server2;bgb=1006.50995,692.76");

        //# Node specs
        string gateway = default("upf");

        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        *.routingTableModule = default("^.ipv4.routingTable");

        //# Transport layer
        bool hasTcp = default(false);
        bool hasOspf = default(true);	// Enable OSPFv2 routing protocol


        int numExtEthInterfaces = default(0); //

    gates:

        //to external networks (internet)
        inout ppp[];

    submodules:

        interfaceTable: InterfaceTable {
            @display("p=61.344,74.976;is=s");
        }

        ospf: MecOspf if hasOspf {
            parameters:
                @display("p=446.832,45.935997");
        }

        //#
        //# ppp modules
        //#
        //  pppIf[sizeof(ppp)]: PppInterface {
        //      @display("p=550,508");
        //  }
        pppIf: PppInterface {
            @display("p=446.832,506.688");
        }

        // ethernet interface used for communicating with external applications in emulation mode
        extEth[numExtEthInterfaces]: <default("ExtLowerEthernetInterface")> like IEthernetInterface {
            parameters:
                @display("p=641.712,506.688,row,150;q=txQueue");
        }

        //#
        //# Network layer module
        //#
        ipv4: Ipv4NetworkLayer if hasIpv4 {    //  like INetworkLayer
            parameters:
                @display("p=420.28,372.88;q=queue");
        }
        ipv6: Ipv6NetworkLayer if hasIpv6 {
            parameters:
                @display("p=657.28,374.46;q=queue");
        }

        //#
        //# Transport layer modules and corresponding application modules
        //#
		// =============== UDP ============== //
        scheduler: Scheduler {
            @display("p=551.232,45.935997");
        }

        udp: Udp {
            @display("p=459,217");
        }

        tcp: Tcp if hasTcp {
            @display("p=627,217");
        }

        // message dispatcher for SAP between application and transport layer
        at: MessageDispatcher {
            parameters:
                @display("p=550,146;b=600,5,,,,1");
        }
        // message dispatcher for SAP between transport and network layer
        tn: MessageDispatcher {
            parameters:
                @display("p=550,296;b=600,5,,,,1");
        }
        // message dispatcher for SAP to link layer
        nl: MessageDispatcher {
            parameters:
                @display("p=550,446;b=600,5,,,,1");
        }

    connections allowunconnected:

        //# 
        //# UALCMP to transport layer connections
        //#        
        scheduler.socketOut --> at.in++;
        scheduler.socketIn <-- at.out++;

        at.out++ --> udp.appIn;
        at.in++ <-- udp.appOut;

        at.out++ --> tcp.appIn if hasTcp;
        at.in++ <-- tcp.appOut if hasTcp;


        //# 
        //# Transport layer to network layer connections
        //#
        udp.ipOut --> tn.in++;
        udp.ipIn <-- tn.out++;

        tcp.ipOut --> tn.in++ if hasTcp;
        tcp.ipIn <-- tn.out++ if hasTcp;


        ipv4.ifIn <-- nl.out++ if hasIpv4;
        ipv4.ifOut --> nl.in++ if hasIpv4;

        ipv4.transportIn <-- tn.out++ if hasIpv4;
        ipv4.transportOut --> tn.in++ if hasIpv4;

        ipv6.ifIn <-- nl.out++ if hasIpv6;
        ipv6.ifOut --> nl.in++ if hasIpv6;

        ipv6.transportIn <-- tn.out++ if hasIpv6;
        ipv6.transportOut --> tn.in++ if hasIpv6;
        //
        //  for i=0..sizeof(ppp)-1 {
        //      pppIf[i].upperLayerOut --> nl.in++;
        //      pppIf[i].upperLayerIn <-- nl.out++;
        //      pppIf[i].phys <--> gtpEndpoint.pppg++;
        //  }

        pppIf.upperLayerOut --> nl.in++;
        pppIf.upperLayerIn <-- nl.out++;
        pppIf.phys <--> ppp++;

        for i=0..numExtEthInterfaces-1 {
            extEth[i].upperLayerOut --> nl.in++;
            extEth[i].upperLayerIn <-- nl.out++;
        }

        ospf.socketOut --> at.in++;
        ospf.socketIn <-- at.out++;

}



