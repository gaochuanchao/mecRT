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

#include "mecrt/nic/mac/allocator/GnbAllocationModule.h"
#include "mecrt/nic/mac/GnbMac.h"

using namespace omnetpp;
using namespace inet;

GnbAllocationModule::GnbAllocationModule(LteMacEnb *mac, Direction direction)
    : LteAllocationModule(mac,direction)
{
    rbPerBand_ = check_and_cast<GnbMac*>(mac)->getRbPerBand();
    EV << NOW << " GnbAllocationModule::GnbAllocationModule - number of resource blocks per Band " << rbPerBand_ << endl;
}

unsigned int GnbAllocationModule::availableBlocks(const MacNodeId nodeId, const Remote antenna, const Band band)
{
    Plane plane = getOFDMPlane(nodeId);

    // blocks allocated in the current band
    unsigned int allocatedBlocks = allocatedRbsPerBand_[plane][antenna][band].allocated_;

    if (allocatedBlocks == 0)
    {
        EV << NOW << " GnbAllocationModule::availableBlocks " << dirToA(dir_) << " - Band " << band << " has " << rbPerBand_ <<" block available" << endl;
        return rbPerBand_;
    }

    // no space available on current antenna.
    return 0;
}

bool GnbAllocationModule::addBlocks(const Remote antenna, const Band band, const MacNodeId nodeId,
    const unsigned int blocks, const unsigned int bytes)
{
    // Check if the band exists
    if (band >= bands_)
        throw cRuntimeError("GnbAllocationModule::addBlocks(): Invalid band %d", (int) band);

    // Check if there's enough OFDM space
    // retrieving user's plane
    Plane plane = getOFDMPlane(nodeId);

    // Obtain the available blocks on the given band
    int availableBlocksOnBand = availableBlocks(nodeId, antenna, band);

    // Check if the band can satisfy the request
    if (availableBlocksOnBand == 0)
    {
        EV << NOW << " GnbAllocationModule::addBlocks " << dirToA(dir_) << " - Node " << nodeId <<
        ", not enough space on band " << band << ": requested " << blocks <<
        " available " << availableBlocksOnBand << " " << endl;
        return false;
    }
        // check if UE is out of range. (CQI=0 => bytes=0)
    if (bytes == 0)
    {
        EV << NOW << " GnbAllocationModule::addBlocks " << dirToA(dir_) << " - Node " << nodeId << " - 0 bytes available with " << blocks << " blocks" << endl;
        return false;
    }

        // Note the request on the allocator structures
    allocatedRbsPerBand_[plane][antenna][band].ueAllocatedRbsMap_[nodeId] += blocks;
    allocatedRbsPerBand_[plane][antenna][band].ueAllocatedBytesMap_[nodeId] += bytes;
    allocatedRbsPerBand_[plane][antenna][band].allocated_ += blocks;

    allocatedRbsUe_[nodeId].ueAllocatedRbsMap_[antenna][band] += blocks;
    allocatedRbsUe_[nodeId].allocatedBlocks_ += blocks;
    allocatedRbsUe_[nodeId].allocatedBytes_ += bytes;
    allocatedRbsUe_[nodeId].antennaAllocatedRbs_[antenna] += blocks;

    // Store the request in the allocationList
    AllocationElem elem;
    elem.resourceBlocks_ = blocks;
    elem.bytes_ = bytes;
    allocatedRbsUe_[nodeId].allocationMap_[antenna][band].push_back(elem);

    // update the allocatedBlocks counter
    allocatedRbsMatrix_[plane][antenna] += blocks;

    usedInLastSlot_ = true;

    EV << NOW << " GnbAllocationModule::addBlocks " << dirToA(dir_) << " - Node " << nodeId << ", the request of " << blocks << " blocks on band " << band << " satisfied" << endl;

    return true;
}
