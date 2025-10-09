// Module: Scheduler
// Type: application
// Creator: Chuanchao Gao
// Description: the scheduler for the MEC system. The scheduler collects the offloading requests from UEs, and
//              schedules the requests periodically based on the scheduling scheme. The bandwidth and computing
//              resources are jointly scheduled to meet the latency requirements of the offloading requests.
package mecRT.mecrt.apps.scheduler;

import inet.applications.contract.IApp;


simple Scheduler like IApp
{
    parameters:
        @display("i=block/app2");
        int localPort = default(3000);
        bool periodicScheduling = default(true);
		double grantAckInterval @unit(s) = default(1s);	// 1s
		double appStopInterval @unit(s) = default(0.05s);	// 50ms
		double connOutdateInterval @unit(s) = default(0.5s);
		double offloadOverhead @unit(s) = default(0.001s);	// data offloading overhead, 1ms
		string scheduleScheme = default("Greedy");
		bool rescheduleAll = default(false);
        bool countExeTime = default(true); // whether to count the execution time of schedule scheme, default is true
		int numerologyIndex = default(0);
        int cuStep = default(1);  // the step for computing units, default is 1
        int rbStep = default(1);  // the step for resource blocks, default is 1
        double srvTimeScale = default(1.0); // the scale for app execution time on servers with full resource, default is 1.0
        bool enableBackhaul = default(false); // whether to enable the backhaul network, default is false
        // the maximum number of hops for task forwarding in the backhaul network
        // default is 1, which means the task can only be forwarded to neighboring RSUs after being offloaded
        int maxHops = default(1);
        double virtualLinkRate = default(10000000); // the rate of the virtual link in the backhaul network, default is 10MBps
        double fairFactor = default(1.0); // the fairness factor for scheduling scheme with forwarding, default is 1.0
        string nodeInfoModulePath = default("^.nodeInfo"); // path to the NodeInfo module

        @signal[schedulingTime](type="double");
        @signal[schemeTime](type="double");
        @signal[instanceGenerateTime](type="double");
        @signal[savedEnergy](type="double");
        @signal[pendingAppCount](type="int");
        @signal[grantedAppCount](type="int");
        @statistic[schedulingTime](title="scheduling time"; source="schedulingTime"; record=vector, mean, sum);
        @statistic[schemeTime](title="scheme execution time"; source="schemeTime"; record=vector, mean, sum);
        @statistic[instanceGenerateTime](title="schedule instance generating time"; source="instanceGenerateTime"; record=vector, mean, sum);
        @statistic[savedEnergy](title="total energy saved"; source="savedEnergy"; record=vector, timeavg, sum);
        @statistic[pendingAppCount](title="number of apps pending to schedule"; source="pendingAppCount"; record=vector, mean, sum);
        @statistic[grantedAppCount](title="number of granted apps"; source="grantedAppCount"; record=vector, mean, sum);

    gates:
        output socketOut;
        input socketIn;
}
