//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// store data related to application execution profiling
//

#include "mecrt/common/Database.h"
#include <fstream>
#include <queue>
// #include <iostream>


Define_Module(Database);

Database::~Database()
{
    vehExeData_.clear();
    rsuExeTime_.clear();
    enableInitDebug_ = false;
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
        maxHops_ = par("maxHops");
        serverExeScale_ = par("serverExeScale");

        loadVehExeDataFromFile();
        loadRsuExeDataFromFile();
        loadRsuPosDataFromFile();
        loadRsuGraphDataFromFile();

        // WATCH_MAP(rsuExeTime_);
        WATCH_MAP(rsuIndex2NodeId_);
        WATCH_MAP(rsuNodeId2Index_);

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



/***
 * load RSU graph data from the file
 * the data is in the following format (each line):
 *    0	 1	0	1	0	0	0	0	0	0	0	0	0	0	0
 * 1 represents a connection between two RSUs, 0 represents no connection
 * k-th row represents the connections of RSU k to other RSUs
 */
void Database::loadRsuGraphDataFromFile()
{
    EV << "Database::loadRsuGraphDataFromFile - loading RSU graph data from file: " << rsuGraphPath_ << endl;
    // Implementation for loading RSU graph data can be added here
    // For now, this is a placeholder function.
    ifstream inputFile(rsuGraphPath_.c_str());
    if (!inputFile.is_open())
    {        
        EV << "Database::loadRsuGraphDataFromFile - Error opening file: " << rsuGraphPath_ << endl;
        return;
    }

    // Read the graph data from the file
    string line;
    vector<vector<int>> adjMatrix;
    while (getline(inputFile, line))
    {
        if (!line.empty())
        {
            vector<int> row;
            istringstream iss(line);
            int value;
            while (iss >> value)
            {
                row.push_back(value);
            }
            adjMatrix.push_back(row);
        }
    }
    inputFile.close();

    // Store the graph data in the member variable
    vector<vector<int>> adjList;
    int numRsus = adjMatrix.size();
    for (int i = 0; i < numRsus; ++i)
    {
        vector<int> row;
        for (int j = 0; j < numRsus; ++j)
        {
            if (adjMatrix[i][j] == 1) // if there is a connection
            {
                row.push_back(j); // add j to the adjacency list of i
            }
        }
        adjList.push_back(row); // store the adjacency list for RSU i
    }

    /***
     * store the RSU hop reachability, i.e., which RSUs can be reached from which RSU with maxHops_ hops
     * {
     *   {rsuId1, {rsuId2: hopCount, rsuId3: hopCount, ...}},
     *   {rsuId4, {rsuId5: hopCount, rsuId6: hopCount, ...}},
     *   ...
     * }
     * using BFS to find the reachable RSUs and their hop counts, store the result in rsuHopReach_
     */
    rsuHopReach_.resize(numRsus);
    for (int startRsu = 0; startRsu < numRsus; startRsu++)
    {
        queue<int> q;
        map<int, int> visited; // map to store the hop count for each RSU
        q.push(startRsu);
        visited[startRsu] = 0; // startRsu is reachable from itself with 0 hops
        while (!q.empty())
        {
            int currentRsu = q.front();
            q.pop();
            int currentHopCount = visited[currentRsu];
            // if the current hop count exceeds the maximum hops, stop the search
            if (currentHopCount >= maxHops_)
                continue;
            // iterate through the neighbors of the current RSU
            for (int neighbor : adjList[currentRsu])
            {
                // if the neighbor is not visited, add it to the queue and mark it as visited
                if (visited.find(neighbor) == visited.end())
                {
                    visited[neighbor] = currentHopCount + 1; // increment the hop count
                    q.push(neighbor);
                }
            }
        }
        // Store the hop reachability information for the current RSU
        rsuHopReach_[startRsu] = visited;
    }
}


void Database::setRsuNodeId2Index(MacNodeId nodeId, int index)
{
    rsuNodeId2Index_[nodeId] = index;
    rsuIndex2NodeId_[index] = nodeId;
}


/***
 * get the map of reachable RSUs from a specific RSU
 * @param rsuId: the RSU MacNodeId
 * @return the map of reachable RSUs and their hop counts
 */
map<MacNodeId, int> Database::getRsuReachableList(MacNodeId rsuNodeId)
{
    map<MacNodeId, int> reachableList;
    if (rsuNodeId2Index_.find(rsuNodeId) == rsuNodeId2Index_.end())
    {
        EV << NOW << " Database::getRsuReachableList - RSU " << rsuNodeId 
           << " not found in the RSU list" << endl;
        return reachableList; // return empty map if RSU not found
    }
    
    int rsuIndex = rsuNodeId2Index_.at(rsuNodeId);
    for (const auto& pair : rsuHopReach_[rsuIndex])
    {
        // pair.first is the index in the RSU list, pair.second is the hop count
        int rsuIndex = pair.first;
        int hopCount = pair.second;
        if (rsuIndex2NodeId_.find(rsuIndex) == rsuIndex2NodeId_.end())
            continue; // skip if RSU index not found

        reachableList[rsuIndex2NodeId_.at(rsuIndex)] = hopCount;
    }
    
    return reachableList;
}


 /***
  * get the number of hops from a specific RSU to another RSU
  * @param startRsu: the starting RSU MacNodeId
  * @param endRsu: the ending RSU MacNodeId
  * @return the number of hops, or -1 if the end RSU is not reachable from the start RSU
  */
int Database::getRsuHopCount(MacNodeId startRsu, MacNodeId endRsu)
{
    int startIndex = rsuNodeId2Index_.at(startRsu);
    int endIndex = rsuNodeId2Index_.at(endRsu);
    auto it = rsuHopReach_[startIndex].find(endIndex);
    if (it != rsuHopReach_[startIndex].end())
    {
        return it->second; // return the hop count
    }
    
    return -1; // end RSU is not reachable from start RSU
}

