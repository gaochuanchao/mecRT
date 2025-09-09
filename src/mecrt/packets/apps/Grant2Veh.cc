//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
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
