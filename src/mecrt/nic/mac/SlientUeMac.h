//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//
// the lte mac stack of the UE. Avoid the message ttiTick_ is trigger for every TTI, 
// consider to remove this module in the future
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
