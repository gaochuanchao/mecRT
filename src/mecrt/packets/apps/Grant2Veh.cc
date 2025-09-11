//
//  Project: mecRT – Mobile Edge Computing Simulator for Real-Time Applications
//  File:    Grant2Veh.msg
//
//  Description:
//    This file implements message Grant2Veh, which is sent from the 5G NIC module of an ES (RSU)
//	  to users for task offloading related information:
//		- start/suspend/stop offloading
//		- bandwidth allocation
//		- data rate
//    This extended message is mainly to pass the resource block allocation information.
//
//  Author:  Gao Chuanchao (Nanyang Technological University)
//  Date:    2025-09-01
//
//  License: Academic Public License -- NOT FOR COMMERCIAL USE
//

#include "mecrt/packets/apps/Grant2Veh.h"


Register_Class(Grant2Veh)

Grant2Veh::Grant2Veh() :
       :: Grant2Veh_Base()
{
    grantedBlocks.clear();
}

Grant2Veh::~Grant2Veh()
{

}


Grant2Veh& Grant2Veh::operator=(const Grant2Veh& other)
{
    if (&other == this)
        return *this;

    ::Grant2Veh_Base::operator=(other);
    copy(other);

    return *this;
}

void Grant2Veh::copy(const Grant2Veh& other)
{
    this->grantedBlocks = other.grantedBlocks;
    this->grantUpdate_ = other.grantUpdate_;
    this->grantStop_ = other.grantStop_;
    this->newGrant_ = other.newGrant_;
    this->pause_ = other.pause_;
}
