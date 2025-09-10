//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// store data related to application execution profiling
//
#ifndef _VEC_DATABASE_H_
#define _VEC_DATABASE_H_

#include <string.h>
#include <omnetpp.h>
#include <inet/common/INETDefs.h>
#include "common/LteCommon.h"

using namespace omnetpp;
using namespace std;

enum VecServiceType
{
    RESNET152,
    VGG16,
    VGG19,
    INCEPTION_V4,
    PEOPLENET_PRUNED,
    SERVICE_COUNTER
};

enum VecResourceType
{
    GPU,
    CPU,
    RESOURCE_COUNTER
};

enum VecDeviceType
{
    RTX3090,
    RTX4090,
    RTX4500,
    DEVICE_COUNTER
};


class Database : public omnetpp::cSimpleModule
{

  protected:
    bool enableInitDebug_;
    double serverExeScale_; // the scale for the server execution time, default is 1.0

    // store the data related to the application execution profiling
    string vehExeDataPath_; // the path to store the vehicle execution data
    string rsuExeDataPath_; // the path to store the RSU execution data
    string rsuPosDataPath_; // the path to store the RSU position data
    string rsuGraphPath_; // the path to store the RSU graph data

    double idlePower_; // the idle power consumption of the device
    double offloadPower_; // the offloading power consumption of the device

    vector<vector<double>> vehExeData_; // store the vehicle execution data
    map<pair<int, int>, double> rsuExeTime_; // store the execution time of the application
    map<int, pair<double, double>> rsuPosData_; // store the RSU position data

    // typedef unsigned short MacNodeId
    map<MacNodeId, int> rsuNodeId2Index_; // map the RSU node id to the index in RSU list
    map<int, MacNodeId> rsuIndex2NodeId_; // map the RSU index in RSU list to the node id
    vector<map<int, int>> rsuHopReach_; // store the RSU hop reachability, i.e., which RSUs can be reached from which RSU
    int maxHops_; // the maximum number of hops for task forwarding in the backhaul network

  public:
    // define a map for application deadline
    const map<int, double> appDeadline = {
        {RESNET152, 0.07}, // 70ms
        {VGG16, 0.08},    // 80ms
        {VGG19, 0.1},     // 100ms
        {INCEPTION_V4, 0.1}, // 100ms
        {PEOPLENET_PRUNED, 0.09} // 90ms
    };

    // define a map for application name
    const map<int, string> appType = {
        {RESNET152, "RESNET152"},
        {VGG16, "VGG16"},
        {VGG19, "VGG19"},
        {INCEPTION_V4, "INCEPTION_V4"},
        {PEOPLENET_PRUNED, "PEOPLENET_PRUNED"}
    };

    // define a map for device name
    const map<int, string> deviceType = {
        {RTX3090, "RTX3090"},
        {RTX4090, "RTX4090"},
        {RTX4500, "RTX4500"}
    };

    // define a map for resource name
    const map<int, string> resourceType = {
        {GPU, "GPU"},
        {CPU, "CPU"}
    };

  public:
    Database(){};
    ~Database();

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    // load the vehicle execution data from the file
    virtual void loadVehExeDataFromFile();

    // load the RSU execution data from the file
    virtual void loadRsuExeDataFromFile();

    // load RSU position data from the file
    virtual void loadRsuPosDataFromFile();

    // load RSU graph data from the file
    virtual void loadRsuGraphDataFromFile();

    // get the vehicle execution data
    virtual vector<double>& getVehExeData(int idx);

    // get the number of vehicle execution data
    virtual int getNumVehExeData() const { return vehExeData_.size(); }

    // get the RSU execution data
    virtual double getRsuExeTime(int appType, int deviceType);

    // get the RSU position data
    virtual pair<double, double> getRsuPosData(int rsuId);

    // set the RSU node id to index mapping
    virtual void setRsuNodeId2Index(MacNodeId nodeId, int index);

    // get the map of reachable RSUs from a specific RSU
    virtual map<MacNodeId, int> getRsuReachableList(MacNodeId rsuId);

    // get the number of hops from a specific RSU to another RSU
    virtual int getRsuHopCount(MacNodeId startRsu, MacNodeId endRsu);
};

#endif
