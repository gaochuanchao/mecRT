// 
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
// 
// 

package mecRT.mecrt.nic.pdcp_rrc;

import simu5g.stack.pdcp_rrc.NRPdcpRrcUe;
import simu5g.stack.pdcp_rrc.NRPdcpRrcEnb;

//
// UE PDCP/RRC layer of NR stack
//
simple MecPdcpRrcUe extends NRPdcpRrcUe
{
    parameters:
        @class("MecPdcpRrcUe");
}

//
// gNodeB PDCP/RRC layer of NR stack
//
simple MecPdcpRrcEnb extends NRPdcpRrcEnb {
    parameters:
        @class("MecPdcpRrcEnb");
}
