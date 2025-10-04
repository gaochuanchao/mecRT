// 
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
// 
// 

package mecRT.mecrt.nic.phy;
import simu5g.stack.phy.LtePhy;

//
// a single layer of NRPhy module in simu5g
// simulate the PHY stack of the NIC module of UE
// LtePhy --> LtePhyBase --> LtePhyUe --> LtePhyUeD2D --> NRPhyUe
//
simple UePhy like LtePhy
{
    parameters:
        @class("UePhy");
        
        bool resAllocateMode = default(true);
        bool srsDistanceCheck = default(false);  // whether checking the distance for SRS transmission
        // the distance for SRS transmission, if srsDistanceCheck is true, the SRS will only be sent to RSUs within this distance
        double srsDistance @unit(m) = default(600m);  
        // the offload power of the whole NIC module, not only the power carriered by the signal
        double offloadPower = default(2210);    // offloading power of vehicle NIC, default 2210mW
        string nodeInfoModulePath = default("^.^.nodeInfo"); // path to the NodeInfo module
        
        // ================================
        // ========== LtePhyBase ========== 
        // ================================
        @display("i=block/wrxtx");
        double ueTxPower = default(26);
        double eNodeBTxPower = default(46);
        double microTxPower = default(30);
        string txDirection = default("OMNI");
        double txAngle = default(0);
        
        
        bool dynamicCellAssociation = default(false);
        
        // switch for handover messages handling on UEs
        bool enableHandover = default(false);
        double handoverLatency @unit(s) = default(0.05s);
        
        // TODO move to LtePhyUeD2D module
        bool enableMulticastD2DRangeCheck = default(false);
        double multicastD2DRange @unit(m) = default(1000m);
        
        // ================================
        // ========== LtePhyUe ============ 
        // ================================
        // if true, get minRssi from PhyPisaData
        bool minRssiDefault = default(true);
        
        // configurable minimum threshold RSSI for attaching to an eNB (meaningful only if minRssiDefault==false)
        double minRssi @unit("dB")= default(-99.0dB);
        
        // =================================
        // ========== LtePhyUeD2D ========== 
        // ================================
        double d2dTxPower = default(26);
        bool d2dMulticastCaptureEffect = default(true);
        string d2dMulticastCaptureEffectFactor = default("RSRP");  // or distance

        @signal[servingCell];
        // @statistic[servingCell](title="ID of the serving eNodeB for the UE"; unit=""; source="servingCell"; record=vector);
        
        //# CQI statistics
        @signal[averageCqiDl];
        @statistic[averageCqiDl](title="Average Cqi reported in DL"; unit="cqi"; source="averageCqiDl"; record=mean);
        @signal[averageCqiUl];
        @statistic[averageCqiUl](title="Average Cqi reported in UL"; unit="cqi"; source="averageCqiUl"; record=mean);
        
        //# D2D CQI statistic
        @signal[averageCqiD2D];
        @statistic[averageCqiD2D](title="Average Cqi reported in D2D"; unit="cqi"; source="averageCqiD2D"; record=mean);
             
    gates:
        input upperGateIn;       // from upper layer
        output upperGateOut;    // to upper layer
        input radioIn;           // gate to receive message via sendDirect
}
