//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    Database.cc / Database.h
//
//  Description:
//    This file implements the database functionality in the MEC system.
//    The Database is responsible for storing and managing the data related to
//    the application execution profiling, including the vehicle and RSU execution
//    data, as well as the vehicle GPS trace.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
#ifndef _MEC_DATABASE_H_
#define _MEC_DATABASE_H_

#include <string.h>
#include <omnetpp.h>
#include <inet/common/INETDefs.h>
#include "common/LteCommon.h"
#include "mecrt/common/NodeInfo.h"

using namespace omnetpp;
using namespace std;

class Database : public omnetpp::cSimpleModule
{

  protected:
    bool enableInitDebug_;
    double serverExeScale_; // the scale for the server execution time, default is 1.0

    // store the data related to the application execution profiling
    string ueExeDataPath_; // the path to store the UE execution data
    string gnbExeDataPath_; // the path to store the gNB execution data
    string gnbPosDataPath_; // the path to store the gNB position data
    string appDataSizePath_; // the path to store the application data size

    double idlePower_; // the idle power consumption of the device
    double offloadPower_; // the offloading power consumption of the device

    map<string, double> ueExeTime_; // store the execution time of the application
    map<string, double> ueAppAccuracy_; // store the application accuracy
    vector<int> appDataSize_; // store the application data size
    map<string, map<string, double>> gnbExeTime_; // store the execution time of the application
    map<string, double> gnbServiceAccuracy_; // store the service accuracy
    set<string> gnbServices_; // store the gNB services
    map<int, pair<double, double>> gnbPosData_; // store the gNB position data
    vector<string> deviceTypes_; // store the device types

    // store the gNB data
    map<int, NodeInfo*> gnbNodeInfo_;
    set<int> gnbNodeIdx_; // store the gNB node ids

    // error injection parameters
    bool linkErrorInjection_;
    double linkErrorProb_;
    bool serverErrorInjection_;
    double serverErrorProb_;
    int numLinks_;
    double failureRecoveryInterval_; // time for failure recovery in seconds
    map<int, int> failedLinkPerGnb_; // store the number of failed links per gNB
    set<int> failedGnbs_; // store the failed gNBs

    omnetpp::cMessage* errorInjectionTimer_; // timer for error injection

    // define a map for application deadline
    const map<string, double> appDeadline = {
        {"resnet18", 0.07}, // 70ms
        {"googlenet", 0.08},    // 80ms
        {"regnet-x-s", 0.1}     // 100ms
    };

  public:
    Database(){};
    ~Database();

    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

    virtual void handleMessage(omnetpp::cMessage *msg) override;

    // load the UE execution data from the file
    virtual void loadUeExeDataFromFile();
    // load the application data size from the file
    virtual void loadAppDataSizeFromFile();
    // load the gNB execution data from the file
    virtual void loadGnbExeDataFromFile();
    // load gNB position data from the file
    virtual void loadGnbPosDataFromFile();

    // get the UE execution time
    virtual double getUeExeTime(string appType);
    virtual double getUeAppAccuracy(string appType);
    virtual int sampleAppDataSize();
    virtual double getAppDeadline(string appType);
    virtual string sampleAppType();

    double getIdlePower() const { return idlePower_; }
    double getOffloadPower() const { return offloadPower_; }
    double getLocalExecPower(string appType);

    // get the gNB execution data
    virtual double getGnbExeTime(string appType, string deviceType);
    virtual double getGnbServiceAccuracy(string appType);
    virtual pair<double, double> getGnbPosData(int gnbId);
    virtual string sampleDeviceType();
    // TODO: change to app dependent service types in the future
    virtual set<string> getGnbServiceTypes() const { return gnbServices_; }


    // inject link error
    virtual void injectLinkError();
    // inject server error
    virtual void injectServerError();

    // register a gNB node info
    virtual void registerGnbNodeInfo(int gnbIndex, NodeInfo* nodeInfo);
};

#endif  // _MEC_DATABASE_H_
