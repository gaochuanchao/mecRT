// 
//  mecRT: A Mobile Edge Computing Simulation Framework
// Authors: Gao Chuanchao (Nanyang Technological University)
// 
// 

package mecRT.mecrt.nic.phy.ChannelModel;

import simu5g.stack.phy.ChannelModel.NRChannelModel_3GPP38_901;

// This model wraps the NRChannelModel_3GPP38_901
// The existing channel model does not support distance > 5000m and cannot be configured
// This wrapper allows us to run simulations with a larger area
simple MecChannelModel extends NRChannelModel_3GPP38_901
{
    parameters:
    @class("MecChannelModel");
}
