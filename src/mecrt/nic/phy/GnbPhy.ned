// 
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
// 
// 

package mecRT.mecrt.nic.phy;
import simu5g.stack.phy.LtePhy;

//
// a single layer of LtePhyEnbD2D module in simu5g
// simulate the PHY stack of the NIC module of gNB
// LtePhy --> LtePhyBase --> LtePhyEnb --> LtePhyEnbD2D
//
simple GnbPhy like LtePhy
{
    parameters:
        @class("GnbPhy");
        
        bool resAllocateMode = default(true);
        
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
        // ========== LtePhyEnb ========== 
        // ================================
        double targetBler = default(0.001);
        double lambdaMinTh = default(0.02);
        double lambdaMaxTh = default(0.2);
        double lambdaRatioTh = default(20);
        
        // ================================
        // ========= LtePhyEnbD2D ========== 
        // ================================
        bool enableD2DCqiReporting = default(true);     
    
    gates:
        input upperGateIn;       // from upper layer
        output upperGateOut;    // to upper layer
        input radioIn;           // gate to receive message via sendDirect    
}
