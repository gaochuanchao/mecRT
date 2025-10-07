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
	if (stage == inet::INITSTAGE_LAST){
		if (getSystemModule()->hasPar("enableInitDebug"))
            enableInitDebug_ = getSystemModule()->par("enableInitDebug").boolValue();
		if (enableInitDebug_)
			std::cout << "NetTopology::initialize - stage: INITSTAGE_LAST - begins" << std::endl;

		EV << "NetTopology::initialize - stage: INITSTAGE_LAST - adjust gnbRouter position" << std::endl;

		cModule *net = getParentModule();
		// get the actual number of UPFs in the current configuration
		numGnb_ = net->par("numGnb").intValue();
		std::string deviceName = par("deviceName").stdstringValue();

		int displayOffsetX = par("displayOffsetX");
		int displayOffsetY = par("displayOffsetY");

		// adjust the display positions of the gnbRouter
		for (int i = 0; i < numGnb_; i++) {
			auto *gnb = getModuleByPath(("gnb[" + std::to_string(i) + "]").c_str());
			// auto *router = getModuleByPath((deviceName +"[" + std::to_string(i) + "]").c_str());

			// Get gnb position from its display string
			cDisplayString& gnbDisp = gnb->getDisplayString();
			char *end;
			double x = strtod(gnbDisp.getTagArg("p", 0), &end);
			double y = strtod(gnbDisp.getTagArg("p", 1), &end);

			// Place the gnbRouter a fixed offset to the right (say +100 in x direction)
			// cDisplayString& routerDisp = router->getDisplayString();
			// routerDisp.setTagArg("p", 0, x + displayOffsetX);  // new x
			// routerDisp.setTagArg("p", 1, y + displayOffsetY);  // new y
		}

		if (enableInitDebug_)
			std::cout << "NetTopology::initialize - stage: INITSTAGE_LAST - ends" << std::endl;
	}
}
