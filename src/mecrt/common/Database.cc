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

#include "mecrt/common/Database.h"
#include <fstream>
#include <queue>
// #include <iostream>


Define_Module(Database);

Database::~Database()
{
    if (enableInitDebug_)
        std::cout << "Database::~Database - destroying Database module\n";

    // vehExeData_.clear();
    // rsuExeTime_.clear();

    if (enableInitDebug_)
        std::cout << "Database::~Database - destroying Database module done!\n";
}


void Database::initialize(int stage)
{
    if (stage == inet::INITSTAGE_LOCAL)
    {
        if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
            std::cout << "Database::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

        ueExeDataPath_ = par("ueExeDataPath").stringValue();
        appDataSizePath_ = par("appDataSizePath").stringValue();
        gnbExeDataPath_ = par("gnbExeDataPath").stringValue();
        gnbPosDataPath_ = par("gnbPosDataPath").stringValue();
        idlePower_ = par("idlePower");
        offloadPower_ = par("offloadPower");
        serverExeScale_ = par("gnbExeScale");
        linkErrorInjection_ = par("linkErrorInjection").boolValue();
        linkErrorProb_ = par("linkErrorProb");
        serverErrorInjection_ = par("serverErrorInjection").boolValue();
        serverErrorProb_ = par("serverErrorProb");
        numLinks_ = par("numLinks");
        failureRecoveryInterval_ = par("failureRecoveryInterval").doubleValue();
        routeUpdate_ = par("routeUpdate").boolValue();

        appDataSize_.clear();
        ueExeTime_.clear();
        ueAppAccuracy_.clear();
        gnbExeTime_.clear();
        gnbServiceAccuracy_.clear();
        deviceTypes_.clear();
        gnbServices_.clear();

        loadAppDataSizeFromFile();
        loadUeExeDataFromFile();
        loadGnbExeDataFromFile();
        loadGnbPosDataFromFile();

        errorInjectionTimer_ = new omnetpp::cMessage("errorInjectionTimer");
        errorInjectionTimer_->setSchedulingPriority(1); // set a lower priority
        scheduleAt(simTime(), errorInjectionTimer_);

        bnResyncTimer_ = new omnetpp::cMessage("bnResyncTimer");
        bnResyncTimer_->setSchedulingPriority(1); // set a lower priority

        WATCH(numLinks_);
        WATCH(linkErrorInjection_);
        WATCH(linkErrorProb_);
        WATCH(serverErrorInjection_);
        WATCH(serverErrorProb_);
        WATCH(failureRecoveryInterval_);
        WATCH_MAP(ueExeTime_);
        WATCH_MAP(ueAppAccuracy_);
        WATCH_MAP(gnbServiceAccuracy_);
        WATCH_MAP(gnbNodeInfo_);
        WATCH_SET(gnbNodeIdx_);
        WATCH_MAP(failedLinkPerGnb_);
        WATCH_SET(failedGnbs_);
        WATCH(routeUpdate_);

        if (enableInitDebug_)
            std::cout << "Database::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
}


void Database::handleMessage(omnetpp::cMessage *msg)
{
    if (msg->isSelfMessage())
    {
        if (msg == errorInjectionTimer_)
        {
            EV << "Database::handleMessage - handling self message: " << msg->getName() << endl;
            // inject link error based on the probability
            injectLinkError();
            // inject server error based on the probability
            injectServerError();

            // reschedule the timer for the next error injection
            scheduleAt(simTime() + failureRecoveryInterval_ * 2, errorInjectionTimer_);

            if (!routeUpdate_ && !bnResyncTimer_->isScheduled())
                scheduleAt(errorRecoverTime_, bnResyncTimer_);
        }
        else if (msg == bnResyncTimer_)
        {
            EV << "Database::handleMessage - handling self message: " << msg->getName() << endl;
            // recover from errors
            recoverFromErrors();
            
            if (simTime() < errorRecoverTime_ && !bnResyncTimer_->isScheduled())
                scheduleAt(errorRecoverTime_, bnResyncTimer_);
        }
    }
    else
    {
        delete msg;
    }
}


void Database::recoverFromErrors()
{
    for (const auto& entry : gnbNodeInfo_)
    {
        int gnbIndex = entry.first;
        NodeInfo* gnbInfo = entry.second;
        if (gnbInfo)
        {
            EV << "Database::recoverFromErrors - Recovering from errors for gNB index: " << gnbIndex << endl;
            gnbInfo->recoverFromErrors();
        }
    }
}


/***
 * inject link error based on the probability
 */
void Database::injectLinkError()
{
    if (linkErrorInjection_)
    {
        failedLinkPerGnb_.clear();
        double numFailedLinks = floor(numLinks_ * linkErrorProb_);
        // sample the failed gnbs
        for (int i = 0; i < numFailedLinks; ++i)
        {
            // randomly select a gNB index from gnbNodeIdx_
            auto it = gnbNodeIdx_.begin();
            std::advance(it, intuniform(0, gnbNodeIdx_.size() - 1));
            int gnbIndex = *it;
            failedLinkPerGnb_[gnbIndex]++;
        }

        // inject link error to each gNB
        for (const auto& entry : failedLinkPerGnb_)
        {            
            int gnbIndex = entry.first;
            int numFailedLinks = entry.second;

            NodeInfo* gnbInfo = gnbNodeInfo_[gnbIndex];
            if (gnbInfo)
            {
                EV << "Database::injectLinkError - Injected " << numFailedLinks << " link errors to gNB index: " << gnbIndex << endl;
                double failedTime = simTime().dbl() + failureRecoveryInterval_;
                double recoverTime = failedTime + failureRecoveryInterval_;
                gnbInfo->injectLinkError(numFailedLinks, failedTime, recoverTime);

                errorRecoverTime_ = recoverTime;
            }
        }
    }
}


void Database::injectServerError()
{
    if (serverErrorInjection_)
    {
        // sample the failed gnbs from gnbNodeIdx_
        failedGnbs_.clear();
        int numFailedGnbs = ceil(gnbNodeIdx_.size() * serverErrorProb_);
        if (numFailedGnbs > gnbNodeIdx_.size())
            throw omnetpp::cRuntimeError("Database::injectServerError - number of failed gNBs exceeds total gNBs!");

        while (failedGnbs_.size() < numFailedGnbs)
        {
            // randomly select a gNB index from gnbNodeIdx_
            auto it = gnbNodeIdx_.begin();
            std::advance(it, intuniform(0, gnbNodeIdx_.size() - 1));
            int gnbIndex = *it;
            failedGnbs_.insert(gnbIndex);
        }
        
        // inject server error to each gNB
        for (const auto& gnbIndex : failedGnbs_)
        {
            NodeInfo* gnbInfo = gnbNodeInfo_[gnbIndex];
            if (gnbInfo)
            {
                EV << "Database::injectServerError - Injected server error to gNB index: " << gnbIndex << endl;
                double failedTime = simTime().dbl() + failureRecoveryInterval_;
                double recoverTime = failedTime + failureRecoveryInterval_;
                gnbInfo->injectNodeError(failedTime, recoverTime);

                errorRecoverTime_ = recoverTime;
            }
        }
    }
}


void Database::registerGnbNodeInfo(int gnbIndex, NodeInfo* nodeInfo)
{
    gnbNodeInfo_[gnbIndex] = nodeInfo;
    gnbNodeIdx_.insert(gnbIndex);
    EV << "Database::registerGnbNodeInfo - Registered gNB node info for index: " << gnbIndex << endl;
}


/***
 * load the application data size from the file
 * data is in following format (single line):
 *      app_data_size(KB)
 */
void Database::loadAppDataSizeFromFile()
{
    EV << "Database::loadAppDataSizeFromFile - loading application data size from file: " << appDataSizePath_ << endl;

    ifstream inputFile(appDataSizePath_.c_str());
    if (!inputFile.is_open())
    {
        EV << "Database::loadAppDataSizeFromFile - Error opening file: " << appDataSizePath_ << endl;
        return;
    }

    string line;
    if (getline(inputFile, line))
    {
        if (!line.empty())
        {
            // Create a string stream from the input string
            istringstream iss(line);
            int appDataSize;
            iss >> appDataSize;
            appDataSize_.push_back(appDataSize);
        }
    }
    inputFile.close();
}

/***
 * load the vehicle execution data from the file
 * data is in following format (each line):
 *      network_name exe_time accuracy
 * store the data in the vector of vector<double>
 */
void Database::loadUeExeDataFromFile()
{
    EV << "Database::loadUeExeDataFromFile - loading UE execution data from file: " << ueExeDataPath_ << endl;

    ifstream inputFile(ueExeDataPath_.c_str());
    if (!inputFile.is_open())
    {
        EV << "Database::loadUeExeDataFromFile - Error opening file: " << ueExeDataPath_ << endl;
        return;
    }

    string line;
    getline(inputFile, line); // skip the header line

    string name;
    double exe_time;
    double accuracy;
    while (getline(inputFile, line))
    {
        if (!line.empty())
        {
            // Create a string stream from the input string
            istringstream iss(line);
            iss >> name >> exe_time >> accuracy;
            ueExeTime_[name] = exe_time / 1000.0; // convert ms to s
            ueAppAccuracy_[name] = accuracy;
        }
    }
    inputFile.close();

    // print the loaded data for debugging
    EV << "\tLoaded UE execution data:" << endl;
    for (const auto& entry : ueExeTime_)
    {
        EV << "\t\tApplication: " << entry.first << ", Execution Time: " << entry.second
           << "s, Accuracy: " << ueAppAccuracy_[entry.first] << endl;
    }
}


/***
 * load the server execution data from the file
 * data is in following format (each line):
 *                   RTX3090   RTX4090   RTX4500    Accuracy
 *      network          *         *         *          *
 * store the data in the vector of vector<double>
 */
void Database::loadGnbExeDataFromFile()
{
    EV << "Database::loadGnbExeDataFromFile - loading gNB execution data from file: " << gnbExeDataPath_ << endl;

    ifstream inputFile(gnbExeDataPath_.c_str());
    if (!inputFile.is_open())
    {
        EV << "Database::loadGnbExeDataFromFile - Error opening file: " << gnbExeDataPath_ << endl;
        return;
    }

    string line;
    getline(inputFile, line); // skip the header line
    
    istringstream iss(line);
    string name, gpu1, gpu2, gpu3, dummy;
    iss >> name >> gpu1 >> gpu2 >> gpu3 >> dummy; // read the header line
    deviceTypes_.push_back(gpu1);
    deviceTypes_.push_back(gpu2);
    deviceTypes_.push_back(gpu3);

    double time1, time2, time3, accuracy;
    while (getline(inputFile, line))
    {
        if (!line.empty())
        {
            
            // Create a string stream from the input string
            istringstream iss(line);
            iss >> name >> time1 >> time2 >> time3 >> accuracy;
            gnbExeTime_[name][gpu1] = time1 / 1000.0; // convert ms to s
            gnbExeTime_[name][gpu2] = time2 / 1000.0;
            gnbExeTime_[name][gpu3] = time3 / 1000.0;
            gnbServiceAccuracy_[name] = accuracy;

            gnbServices_.insert(name);
        }
    }
    inputFile.close();

    // print the loaded data for debugging
    EV << "\tLoaded gNB execution data:" << endl;
    for (const auto& entry : gnbExeTime_)
    {
        EV << "\t\tApplication: " << entry.first 
           << ", Execution Times: [";
        for (const auto& deviceEntry : entry.second)
        {
            EV << deviceEntry.first << ": " << deviceEntry.second << "s, ";
        }
        EV << "]" << endl;
    }

    EV << "\tLoaded gNB service accuracy data:" << endl;
    for (const auto& entry : gnbServiceAccuracy_)
    {
        EV << "\t\tApplication: " << entry.first << ", Service Accuracy: " << entry.second << endl;
    }
}

/***
 * load RSU position data from the file. each line of the data is in the following format:
 *      x_pos, y_pos
 */
void Database::loadGnbPosDataFromFile()
{
    EV << "Database::loadGnbPosDataFromFile - loading gNB position data from file: " << gnbPosDataPath_ << endl;

    ifstream inputFile(gnbPosDataPath_.c_str());
    if (!inputFile.is_open())
    {
        EV << "Database::loadGnbPosDataFromFile - Error opening file: " << gnbPosDataPath_ << endl;
        return;
    }

    string line;
    int gnbId = 0;
    while (getline(inputFile, line))
    {
        if (!line.empty())
        {
            // Create a string stream from the input string
            istringstream iss(line);
            // Variable to store each token (word)
            string x_str, y_str;
            double x_pos, y_pos;
            // get the data of the following form: x_pos, y_pos
            getline(iss, x_str, ','); // get the x position
            getline(iss, y_str); // get the y position
            x_pos = stod(x_str);
            y_pos = stod(y_str);

            gnbPosData_[gnbId] = make_pair(x_pos, y_pos);
            gnbId += 1;
        }
    }
    inputFile.close();
}


/***
 * UE related data access functions
 */
double Database::getUeExeTime(string appType)
{
    return ueExeTime_[appType];
}

double Database::getUeAppAccuracy(string appType)
{
    return ueAppAccuracy_[appType];
}

int Database::sampleAppDataSize()
{
    // return a random image data size from the list
    int index = intuniform(0, appDataSize_.size() - 1);
    return appDataSize_[index];
}

double Database::getAppDeadline(string appType)
{
    // if the appType is not found, return a default value of 0s
    auto it = appDeadline.find(appType);
    if (it != appDeadline.end()) {
        return it->second;
    } else {
        EV << "Database::getAppDeadline - Warning: appType " << appType 
            << " not found in appDeadline map. Returning default deadline of 0s." << endl;
        return 0.0;
    }
}

double Database::getLocalExecPower(string appType)
{
    // TODO: This suppose to be obtained from profiling data
    // Here, we return a dummy value for demonstration purposes
    return idlePower_ + 500.0; // in mW
}

string Database::sampleAppType()
{
    // sample an application type based on a uniform distribution
    int typeId = intuniform(0, appDeadline.size() - 1);
    auto it = appDeadline.begin();
    std::advance(it, typeId);
    return it->first;
}


/***
 * get the gNB related data
 */
double Database::getGnbExeTime(string appType, string deviceType)
{
    return gnbExeTime_[appType][deviceType] * serverExeScale_; // scale the execution time by the server execution scale
}

double Database::getGnbServiceAccuracy(string appType)
{
    return gnbServiceAccuracy_[appType];
}

pair<double, double> Database::getGnbPosData(int gnbId)
{
    return gnbPosData_[gnbId];
}

string Database::sampleDeviceType()
{
    // sample a device type based on a uniform distribution
    int typeId = intuniform(0, deviceTypes_.size() - 1);
    return deviceTypes_[typeId];
}

