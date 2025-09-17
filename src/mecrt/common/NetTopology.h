//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    NetTopology.cc / NetTopology.h
//
//  Description:
//    This file is responsable for the backhaul network topology creation in the MEC system.
//    By reading the network topology from a file, the network topology can be easily modified for different experiments.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-16
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_NETTOPOLOGY_H_
#define _MECRT_NETTOPOLOGY_H_

#include <omnetpp.h>
#include <fstream>
#include <vector>
#include <inet/common/INETDefs.h>
// #include <string.h>

using namespace omnetpp;

class NetTopology : public omnetpp::cSimpleModule
{
  protected:
    bool enableInitDebug_;
    int numGnb_;

  protected:
    virtual void initialize(int stage) override;
    virtual int numInitStages() const override { return inet::NUM_INIT_STAGES; }

  public:
    NetTopology() {}
    ~NetTopology() {}
};

#endif /* _MECRT_NETTOPOLOGY_H_ */

