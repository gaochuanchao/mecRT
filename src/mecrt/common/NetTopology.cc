//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    NetTopology.cc / NetTopology.h
//
//  Description:
//    This file is responsable for the backhaul network topology creation in the MEC system.
//    By reading the network topology from a file, the network topology can be easily modified for different experiments.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/common/NetTopology.h"

Define_Module(NetTopology);

void NetTopology::initialize(int stage)
{
	if (stage == inet::INITSTAGE_LOCAL){
		if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
        if (enableInitDebug_)
			std::cout << "NetTopology::initialize - stage: INITSTAGE_LOCAL - begins" << std::endl;

		std::string linkType_ = par("linkType").stringValue();
		std::string fileName = par("topologyFile").stdstringValue();

		EV << "NetTopology::initialize - Reading UPF topology from " << fileName << endl;

        std::ifstream in(fileName);
        if (!in.is_open())
            throw cRuntimeError("Cannot open topology file: %s", fileName.c_str());

        // try to detect if it's adjacency matrix (n numbers per line)
		// the number of nodes is the size of the first line
		std::string firstLine;
		std::getline(in, firstLine);
		std::istringstream iss(firstLine);
		std::vector<int> firstValues;
		int val;
		while (iss >> val) firstValues.push_back(val);
		int n = firstValues.size();
		if (n == 0)
			throw cRuntimeError("NetTopology::initialize - Invalid first line in topology file: %s", firstLine.c_str());

        std::vector<std::vector<int>> adj(n, std::vector<int>(n, 0));
		for (int j=0; j<n; j++)
			adj[0][j] = firstValues[j];

        std::string line;
        int lineCount = 1; // already read the first line
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue; // skip comments
            std::istringstream iss(line);
            std::vector<int> values;
            while (iss >> val) values.push_back(val);

			if (values.size() == (size_t)n) {	// it is a adjacency matrix
				for (int j=0; j<n; j++)
					adj[lineCount][j] = values[j];
			}
			else {
				throw cRuntimeError("NetTopology::initialize - Invalid line %d in topology file: %s", 
					lineCount+1, line.c_str());
			}

            lineCount++;
        }

        cModule *net = getParentModule();
		// get the actual number of UPFs in the current configuration
		numGnb_ = net->par("numGnb").intValue();
		if (numGnb_ > n)
			throw cRuntimeError("NetTopology::initialize - The number of gnbUpf (%d) exceeds the size of the topology (%d)", numGnb_, n);

        for (int i=0; i < numGnb_; i++) {
            for (int j=i+1; j < numGnb_; j++) {
                if (adj[i][j]) {
                    cModule *a = net->getSubmodule("gnbUpf", i);
                    cModule *b = net->getSubmodule("gnbUpf", j);

                    // add gates if missing
					if (!a->hasGate("pppg"))
						a->addGate("pppg", cGate::INOUT);
					if (!b->hasGate("pppg"))
						b->addGate("pppg", cGate::INOUT);

                    // connect A → B
					auto *chan1 = cChannelType::get(linkType_.c_str())->create("chan");
					a->getOrCreateFirstUnconnectedGate("pppg", 'o', false, true)->connectTo(
						b->getOrCreateFirstUnconnectedGate("pppg", 'i', false, true), chan1);

					// connect B → A
					auto *chan2 = cChannelType::get(linkType_.c_str())->create("chan");
					b->getOrCreateFirstUnconnectedGate("pppg", 'o', false, true)->connectTo(
						a->getOrCreateFirstUnconnectedGate("pppg", 'i', false, true), chan2);

                    EV_INFO << "Connected gnbUpf[" << i << "] <--> gnbUpf[" << j << "]\n";
                }
            }
        }

		if (enableInitDebug_)
			std::cout << "NetTopology::initialize - stage: INITSTAGE_LOCAL - ends" << std::endl;
    }
	else if (stage == inet::INITSTAGE_LAST){
		if (enableInitDebug_)
			std::cout << "NetTopology::initialize - stage: INITSTAGE_LAST - begins" << std::endl;

		int displayOffsetX = par("displayOffsetX");
		int displayOffsetY = par("displayOffsetY");

		// adjust the display positions of the gnbUpf
		for (int i = 0; i < numGnb_; i++) {
			auto *gnb = getModuleByPath(("gnb[" + std::to_string(i) + "]").c_str());
			auto *upf = getModuleByPath(("gnbUpf[" + std::to_string(i) + "]").c_str());

			// Get gnb position from its display string
			cDisplayString& gnbDisp = gnb->getDisplayString();
			char *end;
			double x = strtod(gnbDisp.getTagArg("p", 0), &end);
			double y = strtod(gnbDisp.getTagArg("p", 1), &end);

			// Place the gnbUpf a fixed offset to the right (say +100 in x direction)
			cDisplayString& upfDisp = upf->getDisplayString();
			upfDisp.setTagArg("p", 0, x + displayOffsetX);  // new x
			upfDisp.setTagArg("p", 1, y + displayOffsetY);  // new y
		}

		if (enableInitDebug_)
			std::cout << "NetTopology::initialize - stage: INITSTAGE_LAST - ends" << std::endl;
	}
}
