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

        vehExeDataPath_ = par("vehExeDataPath").stringValue();
        rsuExeDataPath_ = par("rsuExeDataPath").stringValue();
        rsuPosDataPath_ = par("rsuPosDataPath").stringValue();
        rsuGraphPath_ = par("rsuGraphPath").stringValue();
        idlePower_ = par("idlePower");
        offloadPower_ = par("offloadPower");
        serverExeScale_ = par("serverExeScale");

        loadVehExeDataFromFile();
        loadRsuExeDataFromFile();
        loadRsuPosDataFromFile();

        if (enableInitDebug_)
            std::cout << "Database::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
}

/***
 * load the vehicle execution data from the file
 * data is in following format (each line):
 *                    RESNET152        VGG16           VGG19       INCEPTION_V4  PEOPLENET_PRUNED
 * image size (B)|T1 (ms)|P1 (mW)|T2 (ms)|P2 (mW)|T3 (ms)|P3 (mW)|T4 (ms)|P4 (mW)|T5 (ms)|P5 (mW)
 *      e.g.: 74.86	21	1599	28	3067	49	3866	70	4289	79	4576	95	4652
 * store the data in the vector of vector<double>
 */
void Database::loadVehExeDataFromFile()
{
    EV << "Database::loadVehExeDataFromFile - loading vehicle execution data from file: " << vehExeDataPath_ << endl;
    
    ifstream inputFile(vehExeDataPath_.c_str());
    if (!inputFile.is_open())
    {
        EV << "Database::loadVehExeDataFromFile - Error opening file: " << vehExeDataPath_ << endl;
        return;
    }

    vehExeData_.clear();
    string line;
    while (getline(inputFile, line))
    {
        if (!line.empty())
        {
            vector<double> data;
            // Create a string stream from the input string
            istringstream iss(line);
            // Variable to store each token (word)
            string token;
    
            // Read tokens one by one
            while (iss >> token) {
                data.push_back(stod(token));
            }
            vehExeData_.push_back(data);
        }
    }
    inputFile.close();
}


/***
 * load the server execution data from the file
 * data is in following format (each line):
 *                   RTX3090   RTX4090   RTX4500
 *   RESNET152          *         *         *
 *   VGG16              *         *         *
 *   VGG19              *         *         *
 *   INCEPTION_V4       *         *         *
 *   PEOPLENET_PRUNED   *         *         *
 * store the data in the vector of vector<double>
 */
void Database::loadRsuExeDataFromFile()
{
    EV << "Database::loadRsuExeDataFromFile - loading RSU execution data from file: " << rsuExeDataPath_ << endl;
    
    ifstream inputFile(rsuExeDataPath_.c_str());
    if (!inputFile.is_open())
    {
        EV << "Database::loadRsuExeDataFromFile - Error opening file: " << rsuExeDataPath_ << endl;
        return;
    }

    rsuExeTime_.clear();
    string line;
    int apptype = 0;
    while (getline(inputFile, line))
    {
        if (!line.empty())
        {
            vector<double> data;
            // Create a string stream from the input string
            istringstream iss(line);
            // Variable to store each token (word)
            string token;

            // Read tokens one by one
            int deviceType = 0;
            while (iss >> token) {
                double value = stod(token) / 1000.0; // convert to seconds
                rsuExeTime_[make_pair(apptype, deviceType)] = value;
                deviceType += 1;
            }
            apptype += 1;
        }
    }
    inputFile.close();
}

/***
 * load RSU position data from the file. each line of the data is in the following format:
 *      x_pos, y_pos
 */
void Database::loadRsuPosDataFromFile()
{
    EV << "Database::loadRsuPosDataFromFile - loading RSU position data from file: " << rsuPosDataPath_ << endl;
    
    ifstream inputFile(rsuPosDataPath_.c_str());
    if (!inputFile.is_open())
    {
        EV << "Database::loadRsuPosDataFromFile - Error opening file: " << rsuPosDataPath_ << endl;
        return;
    }

    string line;
    int rsuId = 0;
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

            rsuPosData_[rsuId] = make_pair(x_pos, y_pos);
            rsuId += 1;
        }
    }
    inputFile.close();
}


/***
 * get the vehicle execution data
 * @param idx: the index of the data (image index in the file)
 * @return the data
 */
vector<double>& Database::getVehExeData(int idx)
{
    return vehExeData_[idx];
}

/***
 * get the execution time of the application
 * @param appType: the application type
 * @param gpuType: the GPU type
 * @return the execution time
 */
double Database::getRsuExeTime(int appType, int deviceType)
{
    return rsuExeTime_[make_pair(appType, deviceType)] * serverExeScale_; // scale the execution time by the server execution scale
}

/***
 * get the RSU position data
 * @param rsuId: the RSU index in the NED file, i.e., k-th RSU in the RSU list
 * @return the position data
 */
pair<double, double> Database::getRsuPosData(int rsuId)
{
    return rsuPosData_[rsuId];
}



