//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    GnbAllocationModule.cc / GnbAllocationModule.h
//
//  Description:
//    This file implements the bandwidth resource block allocation module for the gNB in the MEC.
//    update the original Allocator Module such that one band can contains multiple RBs.
//    original file: simu5g - "stack/mac/allocator/LteAllocationModule.h"
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#ifndef _MECRT_ALLOCATIONMODULE_H_
#define _MECRT_ALLOCATIONMODULE_H_

#include "common/LteCommon.h"
#include "stack/mac/allocator/LteAllocatorUtils.h"
#include "stack/mac/allocator/LteAllocationModule.h"

class LteMacEnb;

class GnbAllocationModule : public LteAllocationModule
{
  public:
    /// Default constructor.
    GnbAllocationModule(LteMacEnb *mac, const Direction direction);

    /// Destructor.
    virtual ~GnbAllocationModule() { };

    // returns the amount of free blocks for the given band and for the fiven antenna
    virtual unsigned int availableBlocks(const MacNodeId nodeId, const Remote antenna, const Band band);

    // ************** Resource Blocks Allocation Methods **************
    // tries to satisfy the resource block request in the given band and for the fiven antenna
    bool addBlocks(const Remote antenna, const Band band, const MacNodeId nodeId, const unsigned int blocks,
        const unsigned int bytes);

    // --- General Parameters ------------------------------------------------------------------
  protected:

    /// number of resource blocks per band
    unsigned int rbPerBand_;
};

#endif
