//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    SlientUeMac.cc / SlientUeMac.h
//
//  Description:
//    This file implements the MAC layer for the UE in the MEC context.
//    Some UEs may have unused MAC module, which still needs to be checked in each TTI
//    To avoid unnecessary computation, we implement a "silent" MAC module that does nothing
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//
// simulate the MAC stack of the NIC module of gNB
// LteMacBase --> LteMacUe --> SlientUeMac
//

#ifndef _MECRT_SLIENTUEMAC_H_
#define _MECRT_SLIENTUEMAC_H_

#include "stack/mac/layer/LteMacUe.h"

class SlientUeMac : public LteMacUe
{

  protected:
    /**
     * Reads MAC parameters for ue and performs initialization.
     */
    virtual void initialize(int stage) override;

  public:

};

#endif
