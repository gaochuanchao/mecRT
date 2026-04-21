// 
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
// 
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself, 
// and cannot be removed from it.
// vec.simulations.vec.gNodeB;
// vec.simulations.vec.NRUe;
// 


package mecRT.simulations.distributed.map_4;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth100G;
import mecRT.mecrt.NRUe;
import mecRT.mecrt.apps.scheduler.SchedulerHost;
import mecRT.mecrt.common.BandManager;
import mecRT.mecrt.common.Database;
import mecRT.mecrt.coreNetwork.MecUpf;
import mecRT.mecrt.gNodeB;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.world.radio.LteChannelControl;


//
// This network represents the deployment option 3 in 3GPP 38.801
//
network DistMEC
{
    parameters:
        // TODO: Simu5g limites the maximum number of UEs to 1024
        // Specified in simu5g/LteCommon.h, used by Binder to assign MacNodeId to UEs. 
        // In the future, consider to remove the LTE mac and phy from the UE.
        int numUe = default(960);  
        int numGnb = default(100);
        int numBgCells = default(0);
        bool enableInitDebug = default(false); // enable debug info during initialization
        // int npcPort = default(29); // port used by the node packet controller of the gNodeB

        string vehicleTraceFolder; // path of the trace file for the mobility of the UEs
        double areaMinX @unit(m) = default(0m);
        double areaMaxX @unit(m) = default(4800m);
        double areaMinY @unit(m) = default(0m);
        double areaMaxY @unit(m) = default(4800m);
        double areaMinZ @unit(m) = default(0m);
        double areaMaxZ @unit(m) = default(0m);

        @display("i=block/network2;bgb=1319.256,1118.4559;bgi=map");
    submodules:
        channelControl: LteChannelControl {
            @display("p=1211.76,22.44;is=s");
        }
        routingRecorder: RoutingTableRecorder {
            @display("p=1214.004,114.444;is=s");
        }
        configurator: Ipv4NetworkConfigurator {
            @display("p=1214.004,226.64401;is=s");
        }
        binder: Binder {
            @display("p=1214.004,329.868;is=s");
        }
        carrierAggregation: CarrierAggregation {
            @display("p=1214.004,426.36002;is=s");
        }
        database: Database {
            @display("p=1214.004,534.072;is=s");
            numLinks = 180;
        }
        bandManager: BandManager {
            @display("p=1214.004,639.54004;is=s");
        }
        gnb[numGnb]: gNodeB {
            @display("p=642.56,536.136;is=l");
            mobility.constraintAreaMinX = parent.areaMinX;
            mobility.constraintAreaMinY = parent.areaMinY;
            mobility.constraintAreaMaxX = parent.areaMaxX;
            mobility.constraintAreaMaxY = parent.areaMaxY;
            mobility.constraintAreaMinZ = parent.areaMinZ;
            mobility.constraintAreaMaxZ = parent.areaMaxZ;
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=1014.04,702.8;is=l");
        }
        ue[numUe]: NRUe {
            // @display("i=misc/car3;p=642.56,771.07196;is=n;or");
            @display("i=misc/car3;p=642.56,771.07196");
            mobility.constraintAreaMinX = parent.areaMinX;
            mobility.constraintAreaMinY = parent.areaMinY;
            mobility.constraintAreaMaxX = parent.areaMaxX;
            mobility.constraintAreaMaxY = parent.areaMaxY;
            mobility.constraintAreaMinZ = parent.areaMinZ;
            mobility.constraintAreaMaxZ = parent.areaMaxZ;
        }
    connections allowunconnected:
        gnb[0].pppg++ <--> Eth100G <--> gnb[1].pppg++;
        gnb[0].pppg++ <--> Eth100G <--> gnb[2].pppg++;
        gnb[0].pppg++ <--> Eth100G <--> gnb[7].pppg++;
        gnb[0].pppg++ <--> Eth100G <--> gnb[8].pppg++;
        gnb[1].pppg++ <--> Eth100G <--> gnb[34].pppg++;
        gnb[1].pppg++ <--> Eth100G <--> gnb[36].pppg++;
        gnb[1].pppg++ <--> Eth100G <--> gnb[47].pppg++;
        gnb[2].pppg++ <--> Eth100G <--> gnb[9].pppg++;
        gnb[2].pppg++ <--> Eth100G <--> gnb[12].pppg++;
        gnb[2].pppg++ <--> Eth100G <--> gnb[28].pppg++;
        gnb[3].pppg++ <--> Eth100G <--> gnb[6].pppg++;
        gnb[3].pppg++ <--> Eth100G <--> gnb[63].pppg++;
        gnb[4].pppg++ <--> Eth100G <--> gnb[75].pppg++;
        gnb[4].pppg++ <--> Eth100G <--> gnb[82].pppg++;
        gnb[4].pppg++ <--> Eth100G <--> gnb[96].pppg++;
        gnb[5].pppg++ <--> Eth100G <--> gnb[87].pppg++;
        gnb[5].pppg++ <--> Eth100G <--> gnb[88].pppg++;
        gnb[6].pppg++ <--> Eth100G <--> gnb[92].pppg++;
        gnb[6].pppg++ <--> Eth100G <--> gnb[98].pppg++;
        gnb[7].pppg++ <--> Eth100G <--> gnb[21].pppg++;
        gnb[7].pppg++ <--> Eth100G <--> gnb[38].pppg++;
        gnb[7].pppg++ <--> Eth100G <--> gnb[48].pppg++;
        gnb[8].pppg++ <--> Eth100G <--> gnb[27].pppg++;
        gnb[8].pppg++ <--> Eth100G <--> gnb[36].pppg++;
        gnb[8].pppg++ <--> Eth100G <--> gnb[46].pppg++;
        gnb[8].pppg++ <--> Eth100G <--> gnb[64].pppg++;
        gnb[9].pppg++ <--> Eth100G <--> gnb[10].pppg++;
        gnb[9].pppg++ <--> Eth100G <--> gnb[12].pppg++;
        gnb[9].pppg++ <--> Eth100G <--> gnb[38].pppg++;
        gnb[10].pppg++ <--> Eth100G <--> gnb[13].pppg++;
        gnb[10].pppg++ <--> Eth100G <--> gnb[15].pppg++;
        gnb[10].pppg++ <--> Eth100G <--> gnb[39].pppg++;
        gnb[11].pppg++ <--> Eth100G <--> gnb[15].pppg++;
        gnb[11].pppg++ <--> Eth100G <--> gnb[40].pppg++;
        gnb[11].pppg++ <--> Eth100G <--> gnb[53].pppg++;
        gnb[12].pppg++ <--> Eth100G <--> gnb[13].pppg++;
        gnb[12].pppg++ <--> Eth100G <--> gnb[30].pppg++;
        gnb[13].pppg++ <--> Eth100G <--> gnb[14].pppg++;
        gnb[13].pppg++ <--> Eth100G <--> gnb[30].pppg++;
        gnb[13].pppg++ <--> Eth100G <--> gnb[31].pppg++;
        gnb[14].pppg++ <--> Eth100G <--> gnb[15].pppg++;
        gnb[14].pppg++ <--> Eth100G <--> gnb[31].pppg++;
        gnb[14].pppg++ <--> Eth100G <--> gnb[83].pppg++;
        gnb[15].pppg++ <--> Eth100G <--> gnb[24].pppg++;
        gnb[15].pppg++ <--> Eth100G <--> gnb[39].pppg++;
        gnb[16].pppg++ <--> Eth100G <--> gnb[80].pppg++;
        gnb[16].pppg++ <--> Eth100G <--> gnb[89].pppg++;
        gnb[16].pppg++ <--> Eth100G <--> gnb[92].pppg++;
        gnb[17].pppg++ <--> Eth100G <--> gnb[18].pppg++;
        gnb[17].pppg++ <--> Eth100G <--> gnb[33].pppg++;
        gnb[17].pppg++ <--> Eth100G <--> gnb[34].pppg++;
        gnb[17].pppg++ <--> Eth100G <--> gnb[58].pppg++;
        gnb[18].pppg++ <--> Eth100G <--> gnb[19].pppg++;
        gnb[18].pppg++ <--> Eth100G <--> gnb[32].pppg++;
        gnb[18].pppg++ <--> Eth100G <--> gnb[33].pppg++;
        gnb[18].pppg++ <--> Eth100G <--> gnb[34].pppg++;
        gnb[18].pppg++ <--> Eth100G <--> gnb[62].pppg++;
        gnb[19].pppg++ <--> Eth100G <--> gnb[32].pppg++;
        gnb[19].pppg++ <--> Eth100G <--> gnb[35].pppg++;
        gnb[19].pppg++ <--> Eth100G <--> gnb[62].pppg++;
        gnb[20].pppg++ <--> Eth100G <--> gnb[35].pppg++;
        gnb[20].pppg++ <--> Eth100G <--> gnb[45].pppg++;
        gnb[20].pppg++ <--> Eth100G <--> gnb[78].pppg++;
        gnb[20].pppg++ <--> Eth100G <--> gnb[86].pppg++;
        gnb[21].pppg++ <--> Eth100G <--> gnb[22].pppg++;
        gnb[21].pppg++ <--> Eth100G <--> gnb[48].pppg++;
        gnb[21].pppg++ <--> Eth100G <--> gnb[50].pppg++;
        gnb[22].pppg++ <--> Eth100G <--> gnb[23].pppg++;
        gnb[22].pppg++ <--> Eth100G <--> gnb[49].pppg++;
        gnb[22].pppg++ <--> Eth100G <--> gnb[50].pppg++;
        gnb[23].pppg++ <--> Eth100G <--> gnb[69].pppg++;
        gnb[23].pppg++ <--> Eth100G <--> gnb[71].pppg++;
        gnb[23].pppg++ <--> Eth100G <--> gnb[72].pppg++;
        gnb[23].pppg++ <--> Eth100G <--> gnb[79].pppg++;
        gnb[24].pppg++ <--> Eth100G <--> gnb[40].pppg++;
        gnb[24].pppg++ <--> Eth100G <--> gnb[51].pppg++;
        gnb[25].pppg++ <--> Eth100G <--> gnb[26].pppg++;
        gnb[25].pppg++ <--> Eth100G <--> gnb[40].pppg++;
        gnb[25].pppg++ <--> Eth100G <--> gnb[41].pppg++;
        gnb[26].pppg++ <--> Eth100G <--> gnb[52].pppg++;
        gnb[26].pppg++ <--> Eth100G <--> gnb[70].pppg++;
        gnb[27].pppg++ <--> Eth100G <--> gnb[28].pppg++;
        gnb[27].pppg++ <--> Eth100G <--> gnb[65].pppg++;
        gnb[27].pppg++ <--> Eth100G <--> gnb[68].pppg++;
        gnb[28].pppg++ <--> Eth100G <--> gnb[37].pppg++;
        gnb[29].pppg++ <--> Eth100G <--> gnb[30].pppg++;
        gnb[29].pppg++ <--> Eth100G <--> gnb[37].pppg++;
        gnb[29].pppg++ <--> Eth100G <--> gnb[66].pppg++;
        gnb[30].pppg++ <--> Eth100G <--> gnb[31].pppg++;
        gnb[30].pppg++ <--> Eth100G <--> gnb[66].pppg++;
        gnb[30].pppg++ <--> Eth100G <--> gnb[67].pppg++;
        gnb[31].pppg++ <--> Eth100G <--> gnb[67].pppg++;
        gnb[31].pppg++ <--> Eth100G <--> gnb[83].pppg++;
        gnb[31].pppg++ <--> Eth100G <--> gnb[84].pppg++;
        gnb[32].pppg++ <--> Eth100G <--> gnb[55].pppg++;
        gnb[32].pppg++ <--> Eth100G <--> gnb[56].pppg++;
        gnb[33].pppg++ <--> Eth100G <--> gnb[57].pppg++;
        gnb[33].pppg++ <--> Eth100G <--> gnb[58].pppg++;
        gnb[34].pppg++ <--> Eth100G <--> gnb[47].pppg++;
        gnb[34].pppg++ <--> Eth100G <--> gnb[58].pppg++;
        gnb[35].pppg++ <--> Eth100G <--> gnb[49].pppg++;
        gnb[36].pppg++ <--> Eth100G <--> gnb[43].pppg++;
        gnb[36].pppg++ <--> Eth100G <--> gnb[64].pppg++;
        gnb[37].pppg++ <--> Eth100G <--> gnb[65].pppg++;
        gnb[37].pppg++ <--> Eth100G <--> gnb[66].pppg++;
        gnb[38].pppg++ <--> Eth100G <--> gnb[39].pppg++;
        gnb[38].pppg++ <--> Eth100G <--> gnb[50].pppg++;
        gnb[39].pppg++ <--> Eth100G <--> gnb[51].pppg++;
        gnb[41].pppg++ <--> Eth100G <--> gnb[51].pppg++;
        gnb[41].pppg++ <--> Eth100G <--> gnb[52].pppg++;
        gnb[42].pppg++ <--> Eth100G <--> gnb[54].pppg++;
        gnb[42].pppg++ <--> Eth100G <--> gnb[65].pppg++;
        gnb[42].pppg++ <--> Eth100G <--> gnb[68].pppg++;
        gnb[43].pppg++ <--> Eth100G <--> gnb[58].pppg++;
        gnb[44].pppg++ <--> Eth100G <--> gnb[57].pppg++;
        gnb[44].pppg++ <--> Eth100G <--> gnb[59].pppg++;
        gnb[45].pppg++ <--> Eth100G <--> gnb[69].pppg++;
        gnb[46].pppg++ <--> Eth100G <--> gnb[64].pppg++;
        gnb[46].pppg++ <--> Eth100G <--> gnb[68].pppg++;
        gnb[46].pppg++ <--> Eth100G <--> gnb[99].pppg++;
        gnb[47].pppg++ <--> Eth100G <--> gnb[48].pppg++;
        gnb[47].pppg++ <--> Eth100G <--> gnb[49].pppg++;
        gnb[48].pppg++ <--> Eth100G <--> gnb[49].pppg++;
        gnb[48].pppg++ <--> Eth100G <--> gnb[62].pppg++;
        gnb[49].pppg++ <--> Eth100G <--> gnb[69].pppg++;
        gnb[50].pppg++ <--> Eth100G <--> gnb[51].pppg++;
        gnb[50].pppg++ <--> Eth100G <--> gnb[72].pppg++;
        gnb[52].pppg++ <--> Eth100G <--> gnb[72].pppg++;
        gnb[52].pppg++ <--> Eth100G <--> gnb[85].pppg++;
        gnb[53].pppg++ <--> Eth100G <--> gnb[77].pppg++;
        gnb[53].pppg++ <--> Eth100G <--> gnb[83].pppg++;
        gnb[54].pppg++ <--> Eth100G <--> gnb[68].pppg++;
        gnb[54].pppg++ <--> Eth100G <--> gnb[74].pppg++;
        gnb[54].pppg++ <--> Eth100G <--> gnb[75].pppg++;
        gnb[54].pppg++ <--> Eth100G <--> gnb[82].pppg++;
        gnb[55].pppg++ <--> Eth100G <--> gnb[56].pppg++;
        gnb[55].pppg++ <--> Eth100G <--> gnb[59].pppg++;
        gnb[55].pppg++ <--> Eth100G <--> gnb[61].pppg++;
        gnb[55].pppg++ <--> Eth100G <--> gnb[78].pppg++;
        gnb[56].pppg++ <--> Eth100G <--> gnb[57].pppg++;
        gnb[56].pppg++ <--> Eth100G <--> gnb[59].pppg++;
        gnb[57].pppg++ <--> Eth100G <--> gnb[58].pppg++;
        gnb[57].pppg++ <--> Eth100G <--> gnb[80].pppg++;
        gnb[58].pppg++ <--> Eth100G <--> gnb[73].pppg++;
        gnb[59].pppg++ <--> Eth100G <--> gnb[60].pppg++;
        gnb[61].pppg++ <--> Eth100G <--> gnb[78].pppg++;
        gnb[61].pppg++ <--> Eth100G <--> gnb[97].pppg++;
        gnb[63].pppg++ <--> Eth100G <--> gnb[91].pppg++;
        gnb[63].pppg++ <--> Eth100G <--> gnb[98].pppg++;
        gnb[64].pppg++ <--> Eth100G <--> gnb[73].pppg++;
        gnb[65].pppg++ <--> Eth100G <--> gnb[66].pppg++;
        gnb[66].pppg++ <--> Eth100G <--> gnb[75].pppg++;
        gnb[67].pppg++ <--> Eth100G <--> gnb[76].pppg++;
        gnb[69].pppg++ <--> Eth100G <--> gnb[79].pppg++;
        gnb[71].pppg++ <--> Eth100G <--> gnb[94].pppg++;
        gnb[72].pppg++ <--> Eth100G <--> gnb[94].pppg++;
        gnb[73].pppg++ <--> Eth100G <--> gnb[89].pppg++;
        gnb[73].pppg++ <--> Eth100G <--> gnb[90].pppg++;
        gnb[73].pppg++ <--> Eth100G <--> gnb[99].pppg++;
        gnb[74].pppg++ <--> Eth100G <--> gnb[81].pppg++;
        gnb[74].pppg++ <--> Eth100G <--> gnb[91].pppg++;
        gnb[75].pppg++ <--> Eth100G <--> gnb[76].pppg++;
        gnb[75].pppg++ <--> Eth100G <--> gnb[96].pppg++;
        gnb[76].pppg++ <--> Eth100G <--> gnb[88].pppg++;
        gnb[76].pppg++ <--> Eth100G <--> gnb[96].pppg++;
        gnb[77].pppg++ <--> Eth100G <--> gnb[83].pppg++;
        gnb[77].pppg++ <--> Eth100G <--> gnb[84].pppg++;
        gnb[77].pppg++ <--> Eth100G <--> gnb[87].pppg++;
        gnb[78].pppg++ <--> Eth100G <--> gnb[95].pppg++;
        gnb[80].pppg++ <--> Eth100G <--> gnb[89].pppg++;
        gnb[81].pppg++ <--> Eth100G <--> gnb[90].pppg++;
        gnb[81].pppg++ <--> Eth100G <--> gnb[99].pppg++;
        gnb[83].pppg++ <--> Eth100G <--> gnb[84].pppg++;
        gnb[84].pppg++ <--> Eth100G <--> gnb[87].pppg++;
        gnb[84].pppg++ <--> Eth100G <--> gnb[88].pppg++;
        gnb[85].pppg++ <--> Eth100G <--> gnb[93].pppg++;
        gnb[85].pppg++ <--> Eth100G <--> gnb[94].pppg++;
        gnb[90].pppg++ <--> Eth100G <--> gnb[92].pppg++;
        gnb[90].pppg++ <--> Eth100G <--> gnb[98].pppg++;
        gnb[91].pppg++ <--> Eth100G <--> gnb[98].pppg++;
}
