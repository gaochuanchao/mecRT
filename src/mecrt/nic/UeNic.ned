// 
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
// 
// 

package mecRT.mecrt.nic;

import inet.networklayer.common.NetworkInterface;
import mecRT.mecrt.feedback.MecDlFeedbackGenerator;
import mecRT.mecrt.nic.ip2nic.MecIP2Nic;
import mecRT.mecrt.nic.mac.SlientUeMac;
import mecRT.mecrt.nic.mac.UeMac;
import mecRT.mecrt.nic.pdcp_rrc.MecPdcpRrcUe;
import mecRT.mecrt.nic.phy.UePhy;
import mecRT.mecrt.nic.rlc.MecRlc;
import simu5g.stack.ICellularNic;
import simu5g.stack.packetFlowManager.NRPacketFlowManagerUe;
import simu5g.stack.packetFlowManager.PacketFlowManagerUe;
import simu5g.stack.phy.ChannelModel.LteRealisticChannelModel;
import simu5g.stack.phy.ChannelModel.NRChannelModel_3GPP38_901;
import simu5g.stack.rlc.LteRlc;


//
// a single layer of NRNicEnb module in simu5g
// simulate the the NIC module of UE
// NetworkInterface --> LteNicBase --> LteNicUe --> LteNicUeD2D --> NRNicUe
//
module UeNic extends NetworkInterface like ICellularNic
{
    parameters:
        @class(::inet::NetworkInterface);
        @display("i=block/ifcard;bgb=704,443;bgl=3");
        bool isWireless = true;
        string interfaceTableModule;
        string routingTableModule = default("^.ipv4.routingTable");

        bool hasRNISupport = default(false);
        int numCarriers = default(1);

        string nodeType;
        double processingDelayIn @unit(s) = default(0s);   // additional processing delay for incoming ip packets
        double processingDelayOut @unit(s) = default(0s);   // additional processing delay for outgoing ip packets

        bool d2dCapable = default(true);        // DO NOT CHANGE    // inherit the value from the parent module
        bool dualConnectivityEnabled = default(false);

        string address @mutable = default("auto");

        // ========== LteNicUeD2D ==============
        bool d2dInitialMode = default(false);

        // ========== NRNicUe ==============
        int numNRCarriers = default(1);

        string LtePdcpRrcType = default("NRPdcpRrcUe");

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

    submodules:
        // bridge between radio nic and network layer
        ip2nic: MecIP2Nic {
            nodeType = parent.nodeType;
            interfaceTableModule = parent.interfaceTableModule;
            routingTableModule = parent.routingTableModule;
            @display("p=362,67");
        }
        // PDCP-RRC Layer
        pdcpRrc: MecPdcpRrcUe {
            @display("p=362,141");
        }


		// ========== LteNicUe ==============
        // RLC Layer
        rlc: LteRlc {
            @display("p=362,226");
            d2dCapable = parent.d2dCapable;
        }
        // MAC Layer
        // One of: "LteMacUe", "LteMacEnb", "LteMacUeD2D", "LteMacEnbD2D", "SlientUeMac"
        mac: SlientUeMac {
            interfaceTableModule = parent.interfaceTableModule;
            @display("p=363,314");
        }
        // LtePhy submodule
        phy: UePhy {
            @display("p=363,389");
            // @class(LtePhyType);
        }
		// channel model
        channelModel[numCarriers]: LteRealisticChannelModel {
            @display("p=100,265");
        }
        //# Modules used to take trace of PDCP pkt flow
        packetFlowManager: PacketFlowManagerUe if hasRNISupport {
            @display("p=100,205");
        }
        // Feedback generator submodule
        //    dlFbGen: LteDlFeedbackGenerator {
        //        @display("p=50,50;is=s");
        //    }



        // ========== NRNicUe ==============
        // RLC Layer
        nrRlc: MecRlc {
            @display("p=466,226");
            //d2dCapable = parent.d2dCapable;
        }
        // MAC Layer
        nrMac: UeMac {
            interfaceTableModule = parent.interfaceTableModule;
            @display("p=466,314");
        }
        // PHY Layer
        nrPhy: UePhy {
            @display("p=466,389");
            // @class(LtePhyType);
        }
        // NR Channel Model 
        nrChannelModel[numNRCarriers]: NRChannelModel_3GPP38_901 {
            @display("p=100,389");
        }
        // Feedback generator submodule
        nrDlFbGen: MecDlFeedbackGenerator {
            @display("p=100,52;is=s");
        }
        //# Modules used to take trace of PDCP pkt flow
        nrPacketFlowManager: NRPacketFlowManagerUe if hasRNISupport {
            @display("p=100,326");
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

        // ========== NRNicUe ==============
        //# PDCP <-> RLC
        pdcpRrc.UM_Sap++ <--> nrRlc.UM_Sap;
        pdcpRrc.AM_Sap++ <--> nrRlc.AM_Sap;
        pdcpRrc.TM_Sap++ <--> nrRlc.TM_Sap;

        //# RLC <-> MAC
        nrRlc.RLC_to_MAC --> nrMac.RLC_to_MAC;
        nrRlc.MAC_to_RLC <-- nrMac.MAC_to_RLC;

        //#
        //# Connections from LTE Stack to radio interface
        //#
        nrMac.MAC_to_PHY --> nrPhy.upperGateIn;
        nrMac.PHY_to_MAC <-- nrPhy.upperGateOut;

        //# external: lower connection
        nrRadioIn --> nrPhy.radioIn;
}
