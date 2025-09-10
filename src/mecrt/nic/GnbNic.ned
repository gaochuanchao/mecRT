// 
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
// 
// 

package mecRT.mecrt.nic;

import inet.networklayer.common.NetworkInterface;
import mecRT.mecrt.nic.ip2nic.MecIP2Nic;
import mecRT.mecrt.nic.mac.GnbMac;
import mecRT.mecrt.nic.pdcp_rrc.MecPdcpRrcEnb;
import mecRT.mecrt.nic.phy.GnbPhy;
import mecRT.mecrt.nic.rlc.MecRlc;
import simu5g.stack.ICellularNic;
import simu5g.stack.backgroundTrafficGenerator.BackgroundTrafficGenerator;
import simu5g.stack.compManager.compManagerProportional.LteCompManagerProportional;
import simu5g.stack.dualConnectivityManager.DualConnectivityManager;
import simu5g.stack.handoverManager.LteHandoverManager;
import simu5g.stack.packetFlowManager.NRPacketFlowManagerGnb;
import simu5g.stack.phy.ChannelModel.NRChannelModel_3GPP38_901;
import simu5g.x2.LteX2Manager;


//
// a single layer of NRNicEnb module in simu5g
// simulate the the NIC module of gNB
// NetworkInterface --> LteNicBase --> LteNicEnb --> LteNicEnbD2D --> NRNicEnb
//
module GnbNic extends NetworkInterface like ICellularNic
{
    parameters:

        @class(::inet::NetworkInterface);
        @display("i=block/ifcard;bgb=704,443;bgl=3");
        bool isWireless = true;
        string interfaceTableModule;
        string routingTableModule;
        bool hasRNISupport = default(false);
        string nodeType;
        double processingDelayIn @unit(s) = default(0s);   // additional processing delay for incoming ip packets
        double processingDelayOut @unit(s) = default(0s);   // additional processing delay for outgoing ip packets
        bool d2dCapable = default(true);   // DO NOT CHANGE     // inherit the value from the parent module
        bool dualConnectivityEnabled = default(false);
        int numCarriers = default(1);
        string address @mutable = default("auto");
        // ======== LteNicEnb ========
        bool compEnabled = default(false);

    gates:
        //#
        //# Gates connecting UE/eNB and LTE Stack
        //#
        //# Control Ports
        inout EUTRAN_RRC_Sap;   // Control of RRC Layer
        input upperLayerIn;
        output upperLayerOut;
        input radioIn @loose;  // to receive messages sent using sendDirect()
        input nrRadioIn @loose;// for NR support
        inout x2[] @loose;     // optional x2 manager
        //inout serverGate;

    submodules:
        // bridge between radio nic and network layer
        //ip2nic: <default("IP2Nic")> like INetworkLayer2CellularNic {
        ip2nic: MecIP2Nic {
            nodeType = parent.nodeType;
            interfaceTableModule = parent.interfaceTableModule;
            routingTableModule = parent.routingTableModule;
            @display("p=362,67");
        }
        // PDCP-RRC Layer
        // One of: "LtePdcpRrcUe", "LtePdcpRrcEnb", "LtePdcpRrcUeD2D", "LtePdcpRrcEnbD2D" 
        pdcpRrc: MecPdcpRrcEnb {
            @display("p=362,141");
        }
        // RLC Layer
        rlc: MecRlc {
            @display("p=362,226");
            //d2dCapable = parent.d2dCapable;
        }
        // MAC Layer
        // One of: "LteMacUe", "LteMacEnb", "LteMacUeD2D", "LteMacEnbD2D", "GnbMac"
        mac: GnbMac {
            interfaceTableModule = parent.interfaceTableModule;
            @display("p=363,314");
        }
        // LtePhy submodule
        phy: GnbPhy {
            @display("p=363,389");
            @class(LtePhyType);
        }

        channelModel[numCarriers]: NRChannelModel_3GPP38_901 {
            @display("p=100,389");
        }

        //#
        //# Modules used to take trace of PDCP pkt flow
        //#
        packetFlowManager: NRPacketFlowManagerGnb if hasRNISupport {
            @display("p=100,235");
        }

        // ======== LteNicEnb ========
        //#
        //# X2 manager
        //#       
        x2Manager: LteX2Manager {
            @display("p=623,226,row");
        }

        //#
        //# Modules exploiting X2 interface
        //#
        compManager: LteCompManagerProportional if compEnabled {
            @display("p=493,314,row");
        }
        handoverManager: LteHandoverManager {
            @display("p=493,226,row");
        }
        dualConnectivityManager: DualConnectivityManager if dualConnectivityEnabled {
            @display("p=493,141,row");
        }
        bgTrafficGenerator[numCarriers]: BackgroundTrafficGenerator {
            @display("p=100,307,row");
        }

    connections allowunconnected:
        ip2nic.stackNic <--> pdcpRrc.DataPort;

        //#
        //# Internal LTE Stack Connections 
        //# 

        EUTRAN_RRC_Sap <--> pdcpRrc.EUTRAN_RRC_Sap;

        //# PDCP <-> RLC
        pdcpRrc.UM_Sap++ <--> rlc.UM_Sap;
        pdcpRrc.AM_Sap++ <--> rlc.AM_Sap;
        pdcpRrc.TM_Sap++ <--> rlc.TM_Sap;

        //# RLC <-> MAC
        rlc.RLC_to_MAC --> mac.RLC_to_MAC;
        rlc.MAC_to_RLC <-- mac.MAC_to_RLC;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        mac.MAC_to_PHY --> phy.upperGateIn;
        mac.PHY_to_MAC <-- phy.upperGateOut;

        //# external: lower connection
        radioIn --> phy.radioIn;
        ip2nic.upperLayerOut --> {  delay = parent.processingDelayOut; } --> upperLayerOut;
        upperLayerIn --> {  delay = parent.processingDelayIn; } --> ip2nic.upperLayerIn;

        // ======== LteNicEnb ========
        //# connections between X2 Manager and its users
        compManager.x2ManagerIn <-- x2Manager.dataPort$o++ if compEnabled;
        compManager.x2ManagerOut --> x2Manager.dataPort$i++ if compEnabled;
        handoverManager.x2ManagerIn <-- x2Manager.dataPort$o++;
        handoverManager.x2ManagerOut --> x2Manager.dataPort$i++;
        dualConnectivityManager.x2ManagerIn <-- x2Manager.dataPort$o++ if dualConnectivityEnabled;
        dualConnectivityManager.x2ManagerOut --> x2Manager.dataPort$i++ if dualConnectivityEnabled;

        if ( sizeof(x2) > 0 ) {
            //# connections between X2 Manager and GTP (for handover)
            x2$i[0] --> x2Manager.x2Gtp$i;
            x2$o[0] <-- x2Manager.x2Gtp$o;
        }
        //# connections between X2 Manager and the outer world
        for i=1..sizeof(x2)-1 {
            x2$i[i] --> x2Manager.x2$i++;
            x2$o[i] <-- x2Manager.x2$o++;
        }

}
