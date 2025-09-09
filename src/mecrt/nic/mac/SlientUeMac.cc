//
//                  Simu5G
//
// Authors: Giovanni Nardini, Giovanni Stea, Antonio Virdis (University of Pisa)
//
// This file is part of a software released under the license included in file
// "license.pdf". Please read LICENSE and README files before using it.
// The above files and the present reference are part of the software itself,
// and cannot be removed from it.
//

#include "mecrt/nic/mac/SlientUeMac.h"

Define_Module(SlientUeMac);

void SlientUeMac::initialize(int stage)
{
    LteMacUe::initialize(stage);
    
    if (stage == inet::INITSTAGE_LAST)
    {
        // no need to run this timer for slilent Mac
        cancelAndDelete(ttiTick_);
    }
}
