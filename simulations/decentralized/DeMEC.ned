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


package mecRT.simulations.decentralized;

import inet.networklayer.configurator.ipv4.Ipv4NetworkConfigurator;
import inet.networklayer.ipv4.RoutingTableRecorder;
import inet.node.ethernet.Eth100G;
import mecRT.mecrt.NRUe;
import mecRT.mecrt.apps.scheduler.SchedulerHost;
import mecRT.mecrt.common.BandManager;
import mecRT.mecrt.common.Database;
// import mecRT.mecrt.common.NetTopology;
import mecRT.mecrt.coreNetwork.MecUpf;
import mecRT.mecrt.gNodeB;
import mecRT.mecrt.routing.MecRouter;
import simu5g.common.binder.Binder;
import simu5g.common.carrierAggregation.CarrierAggregation;
import simu5g.nodes.backgroundCell.BackgroundCell;
import simu5g.world.radio.LteChannelControl;


//
// This network represents the deployment option 3 in 3GPP 38.801
//
network DeMEC
{
    parameters:
        int numUe = default(1);
        int numGnb = default(15);
        int numBgCells = default(0);
        bool enableInitDebug = default(false); // enable debug info during initialization
        // int npcPort = default(29); // port used by the node packet controller of the gNodeB
        double scheduleInterval @unit(s) = default(1s); // in TTI, if the scheduler run periodically, specify the interval

        @display("i=block/network2;bgb=1319.256,1118.4559;bgi=background/pisa");
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
        // topologyGen: NetTopology {
        //     @display("p=1214.004,742.76404;is=s");
        // }
        database: Database {
            @display("p=1214.004,534.072;is=s");
        }
        bandManager: BandManager {
            @display("p=1214.004,639.54004;is=s");
        }
        gnb[numGnb]: gNodeB {
            @display("p=642.56,536.136;is=l");
        }
        bgCell[numBgCells]: BackgroundCell {
            @display("p=1014.04,702.8;is=l");
        }
        ue[numUe]: NRUe {
            // @display("i=misc/car3;p=642.56,771.07196;is=n;or");
            @display("i=misc/car3;p=642.56,771.07196");
        }
    connections allowunconnected:
        gnb[0].pppg++ <--> Eth100G <--> gnb[1].pppg++;
        gnb[0].pppg++ <--> Eth100G <--> gnb[3].pppg++;
        gnb[1].pppg++ <--> Eth100G <--> gnb[2].pppg++;
        gnb[1].pppg++ <--> Eth100G <--> gnb[3].pppg++;
        gnb[1].pppg++ <--> Eth100G <--> gnb[4].pppg++;
        gnb[2].pppg++ <--> Eth100G <--> gnb[4].pppg++;
        gnb[3].pppg++ <--> Eth100G <--> gnb[4].pppg++;
        gnb[3].pppg++ <--> Eth100G <--> gnb[6].pppg++;
        gnb[4].pppg++ <--> Eth100G <--> gnb[5].pppg++;
        gnb[4].pppg++ <--> Eth100G <--> gnb[6].pppg++;
        gnb[5].pppg++ <--> Eth100G <--> gnb[6].pppg++;
        gnb[5].pppg++ <--> Eth100G <--> gnb[9].pppg++;
        gnb[6].pppg++ <--> Eth100G <--> gnb[7].pppg++;
        gnb[6].pppg++ <--> Eth100G <--> gnb[8].pppg++;
        gnb[6].pppg++ <--> Eth100G <--> gnb[10].pppg++;
        gnb[7].pppg++ <--> Eth100G <--> gnb[8].pppg++;
        gnb[8].pppg++ <--> Eth100G <--> gnb[10].pppg++;
        gnb[9].pppg++ <--> Eth100G <--> gnb[10].pppg++;
        gnb[9].pppg++ <--> Eth100G <--> gnb[13].pppg++;
        gnb[10].pppg++ <--> Eth100G <--> gnb[11].pppg++;
        gnb[10].pppg++ <--> Eth100G <--> gnb[12].pppg++;
        gnb[10].pppg++ <--> Eth100G <--> gnb[14].pppg++;
        gnb[11].pppg++ <--> Eth100G <--> gnb[12].pppg++;
        gnb[12].pppg++ <--> Eth100G <--> gnb[14].pppg++;
        gnb[13].pppg++ <--> Eth100G <--> gnb[14].pppg++;
}
