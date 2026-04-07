// 
//  mecRT: A Mobile Edge Computing Simulation Framework
// Authors: Gao Chuanchao (Nanyang Technological University)
// 
// 

#ifndef MEC_CHANNELMODEL_H_
#define MEC_CHANNELMODEL_H_

#include "stack/phy/ChannelModel/NRChannelModel_3GPP38_901.h"

// This model wraps the NRChannelModel_3GPP38_901
// The existing channel model does not support distance > 5000m and cannot be configured
// This wrapper allows us to run simulations with a larger area
class MecChannelModel : public NRChannelModel_3GPP38_901
{
  protected:
    double MEC_MAX_VALID_2D_DISTANCE = 5000.0;
    double MEC_OUT_OF_RANGE_ATTENUATION = 1000.0;

  public:
    /*
     * Compute Attenuation caused by pathloss and shadowing (optional)
     *
     * @param nodeid mac node id of UE
     * @param dir traffic direction
     * @param coord position of end point comunication (if dir==UL is the position of UE else is the position of gNodeB)
     */
    virtual double getAttenuation(MacNodeId nodeId, Direction dir, inet::Coord coord, bool cqiDl) override;
};

#endif /* MEC_CHANNELMODEL_H_ */
