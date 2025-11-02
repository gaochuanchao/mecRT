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

package mecRT.mecrt.apps.ue;

import inet.applications.contract.IApp;

simple UeApp like IApp
{
    parameters:
        int localPort = default(-1);
        int destPort = default(3000);
        int inputSize = default(50000);	// 50 KB
        int resultSize = default(0);
        int tos = default(-1); // if not -1, set the Type Of Service (IPv4) / Traffic Class (IPv6) field of sent packets to this value
        double dlScale = default(1.0); // the scale for the deadline of the app, default is 1.0. dl = dl / deadlineScale
        string nodeInfoModulePath = default("^.nodeInfo");
        
        @signal[localProcessCount];
        @statistic[localProcessCount](title="number of locally processed jobs"; source="localProcessCount"; record=sum);
        
        @signal[offloadCount];
        @statistic[offloadCount](title="number of offloaded jobs"; source="offloadCount"; record=sum);

        // @signal[vehSavedEnergy];
        // @statistic[vehSavedEnergy](title="energy saved for vehicles"; unit="mJ"; source="vehSavedEnergy"; record=timeavg,sum);

        // @signal[vehEnergyConsumedIfLocal];
        // @statistic[vehEnergyConsumedIfLocal](title="energy consumed for vehicles for fully local processing"; source="vehEnergyConsumedIfLocal"; record=timeavg,sum);

        @display("i=block/source");
    gates:
        output socketOut;
        input socketIn;
}
