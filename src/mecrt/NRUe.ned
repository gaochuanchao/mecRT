// 
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
// 
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself, 
// and cannot be removed from it.
// 


package mecRT.mecrt;

import inet.common.MessageDispatcher;
import inet.common.packet.recorder.PcapRecorder;
import inet.linklayer.contract.IEthernetInterface;
import inet.linklayer.ethernet.basic.EthernetEncapsulation;
import inet.linklayer.loopback.LoopbackInterface;
import inet.networklayer.common.InterfaceTable;
import inet.networklayer.ipv4.Ipv4NetworkLayer;
import inet.networklayer.ipv6.Ipv6NetworkLayer;
import inet.transportlayer.contract.ISctp;
import inet.transportlayer.contract.ITcp;
import mecRT.mecrt.apps.ue.UeApp;
import mecRT.mecrt.mobility.MecMobility;
import mecRT.mecrt.nic.UeNic;
import mecRT.mecrt.udp.MecUdp;
import simu5g.corenetwork.statsCollector.UeStatsCollector;


// 
// User Equipment Module
//
module NRUe
{
    parameters:
        @networkNode();
        @display("i=device/pocketpc;bgb=858.888,659.568");
        @figure[applicationLayer](type=rectangle; pos=250,6; size=600,130; lineColor=#808080; cornerRadius=5; fillColor=#ffff00; fillOpacity=0.1);
        @figure[applicationLayer.title](type=text; pos=845,11; anchor=ne; text="application layer");
        @figure[transportLayer](type=rectangle; pos=250,156; size=600,130; fillColor=#ff0000; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[transportLayer.title](type=text; pos=845,161; anchor=ne; text="transport layer");
        @figure[networkLayer](type=rectangle; pos=250,306; size=600,130; fillColor=#00ff00; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[networkLayer.title](type=text; pos=845,311; anchor=ne; text="network layer");
        @figure[linkLayer](type=rectangle; pos=250,456; size=600,130; fillColor=#0000ff; lineColor=#808080; cornerRadius=5; fillOpacity=0.1);
        @figure[linkLayer.title](type=text; pos=845,461; anchor=ne; text="link layer");
        @figure[submodules];


        //# Mobility
        // mobility.typename = default("LinearMobility");	// LinearMobility, StationaryMobility

        //# Apps
        int numApps = default(0);   // no of apps.
                                    // Specify the app types in INI file with app[0..1].typename="UdpVideoStreamClient" syntax 

        //# Transport layer
        bool hasUdp = default(true);
        bool hasTcp = default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc") != "");
        bool hasSctp = default(false);

        //# Node specs
        string nodeType = "UE";  // DO NOT CHANGE
        int masterId @mutable;
        int macNodeId @mutable = default(0); // TODO: this is not a real parameter
        int macCellId @mutable = default(0); // TODO: this is not a real parameter

        //# Network Layer specs
        bool hasIpv4 = default(true);
        bool hasIpv6 = default(false);
        *.interfaceTableModule = default(absPath(".interfaceTable"));
        //*.routingTableModule = default("^.ipv4.routingTable");

        //# External Ethernet interfaces
        int numEthInterfaces = default(0);
        string extHostAddress = default("");

        //# Loopback interfaces
        int numLoInterfaces = default(1);

        //# Num of PcapRecorders
        int numPcapRecorders = default(0); // no of PcapRecorders
        bool hasRNISupport = default(false);

        // ======= NRUe ===========
        int nrMasterId @mutable = default(0);
        int nrMacNodeId @mutable = default(0); // TODO: this is not a real parameter
        int nrMacCellId @mutable = default(0); // TODO: this is not a real parameter


    gates:
        input radioIn @directIn;     // connection to master
        // ======= NRUe ===========
        input nrRadioIn @directIn;     // connection to master  

    submodules:

        interfaceTable: InterfaceTable {
            @display("p=127.368004,257.868;is=s");
        }
        mobility: MecMobility {
            @display("p=127.368004,172.26001;is=s");
        }

        //# layer submodules (must be ordered from lower to upper layers)

        //# link layer 
        lo[numLoInterfaces]: LoopbackInterface {
            parameters:
                @display("p=503.736,589.50397,row,150");
        }
        cellularNic: UeNic {
            nodeType = parent.nodeType;
            @display("p=355.152,589.50397");
        }
        //# ethernet interface used for communicating with external applications in emulation mode
        eth[numEthInterfaces]: <default("ExtLowerEthernetInterface")> like IEthernetInterface {  //  like IEthernetInterface 
            parameters:
                @display("p=674.06396,589.50397,row,150;q=txQueue");
        }
        encap: EthernetEncapsulation if typename != "" {    // like IEthernetLayer
            parameters:
                registerProtocol = true;
                @display("p=674.06396,496.488");
        }

        //# network layer
        ipv4: Ipv4NetworkLayer if hasIpv4 {   // like INetworkLayer
            parameters:
                @display("p=375,376;q=queue");
        }
        ipv6: Ipv6NetworkLayer if hasIpv6 {   // like INetworkLayer
            parameters:
                @display("p=525,376;q=queue");
        }

        //# transport layer 
        udp: MecUdp if hasUdp {   // like IUdp
            parameters:
                @display("p=375,226");
        }
        tcp: <default(firstAvailableOrEmpty("Tcp", "TcpLwip", "TcpNsc"))> like ITcp if hasTcp {
            parameters:
                @display("p=525,226");
        }
        sctp: <default(firstAvailableOrEmpty("Sctp"))> like ISctp if hasSctp {
            parameters:
                @display("p=675,226");
        }

        //# app layer 
        app[numApps]: UeApp {
            @display("p=374.47998,72.479996,row,140");
        }

        //# message dispatcher for SAP between application and transport layer
        at: MessageDispatcher {
            parameters:
                @display("p=550,146;b=600,5,,,,1");
        }
        //# message dispatcher for SAP between transport and network layer
        tn: MessageDispatcher {
            parameters:
                @display("p=550,296;b=600,5,,,,1");
        }
        //# message dispatcher for SAP to link layer
        nl: MessageDispatcher {
            parameters:
                @display("p=550,446;b=600,5,,,,1");
        }
        //# PcapRecorders
        pcapRecorder[numPcapRecorders]: PcapRecorder {
            @display("p=126.84,346.69598,r,10");
        }

        //# UeStatsCollector - for MEC
        ueCollector: UeStatsCollector if hasRNISupport {
            @display("p=126.84,588.296;is=s");
        }

    connections allowunconnected:

        //# 
        //# Internal Tcp/Udp applications connections with transport layer
        //#

        for i=0..numApps-1 {
            app[i].socketOut --> at.in++;
            app[i].socketIn <-- at.out++;
        }

        at.out++ --> udp.appIn if hasUdp;
        at.in++ <-- udp.appOut if hasUdp;

        at.out++ --> tcp.appIn if hasTcp;
        at.in++ <-- tcp.appOut if hasTcp;

        at.out++ --> sctp.appIn if hasSctp;
        at.in++ <-- sctp.appOut if hasSctp;

        //# 
		//# Internal transport layer connections to network layer
		//# 

        udp.ipOut --> tn.in++ if hasUdp;
        udp.ipIn <-- tn.out++ if hasUdp;

        tcp.ipOut --> tn.in++ if hasTcp;
        tcp.ipIn <-- tn.out++ if hasTcp;

        sctp.ipOut --> tn.in++ if hasSctp;
        tn.out++ --> sctp.ipIn if hasSctp;

        ipv4.ifIn <-- nl.out++ if hasIpv4;
        ipv4.ifOut --> nl.in++ if hasIpv4;

        ipv4.transportIn <-- tn.out++ if hasIpv4;
        ipv4.transportOut --> tn.in++ if hasIpv4;

        ipv6.ifIn <-- nl.out++ if hasIpv6;
        ipv6.ifOut --> nl.in++ if hasIpv6;

        ipv6.transportIn <-- tn.out++ if hasIpv6;
        ipv6.transportOut --> tn.in++ if hasIpv6;

        tn.out++ --> nl.in++;
        tn.in++ <-- nl.out++;

        //#
        //# Internal link layer connections to network layer  
        //#

        cellularNic.upperLayerOut --> nl.in++;
        cellularNic.upperLayerIn <-- nl.out++;

        for i=0..numEthInterfaces-1 {
            encap.lowerLayerOut --> eth[i].upperLayerIn;
            encap.lowerLayerIn <-- eth[i].upperLayerOut;
        }
        encap.upperLayerOut --> nl.in++ if numEthInterfaces != 0;
        encap.upperLayerIn <-- nl.out++ if numEthInterfaces != 0;

        for i=0..numLoInterfaces-1 {
            nl.out++ --> lo[i].upperLayerIn;
            lo[i].upperLayerOut --> nl.in++;
        }

        //#
        //# Connections between LTE/NR NIC and the radio medium
        //#
        cellularNic.radioIn <-- radioIn;

        // ======= NRUe ===========
        cellularNic.nrRadioIn <-- nrRadioIn;
}

