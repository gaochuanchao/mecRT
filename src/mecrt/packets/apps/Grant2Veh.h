//
//                  simple5G
// Authors: Gao Chuanchao (Nanyang Technological University)
//

#ifndef _VEC_GRANT2VEHICLE_H_
#define _VEC_GRANT2VEHICLE_H_

#include <omnetpp.h>
#include "mecrt/packets/apps/Grant2Veh_Base_m.h"

class Grant2Veh : public Grant2Veh_Base
{
  private:
    void copy(const Grant2Veh& other);

  protected:
    std::map<unsigned short, unsigned int> grantedBlocks;
    bool grantUpdate_ = false;
    bool grantStop_ = false;
    bool newGrant_ = false;
    bool pause_ = false;

  public:
    /**
     * Constructor: base initialization
     * @param name packet name
     * @param kind packet kind
     */
    Grant2Veh();
    virtual ~Grant2Veh();

    /*
     * Operator = : packet copy
     * @param other source packet
     * @return reference to this packet
     */
    Grant2Veh& operator=(const Grant2Veh& other);

    /**
     * Copy constructor: packet copy
     * @param other source packet
     */
    Grant2Veh(const Grant2Veh& other) :
        ::Grant2Veh_Base(other)
    {
        copy(other);
    }

    /**
     * dup() : packet duplicate
     * @return pointer to duplicate packet
     */
    virtual Grant2Veh *dup() const override
    {
        return new Grant2Veh(*this);
    }

    const std::map<unsigned short, unsigned int>& getGrantedBlocks() const
    {
        return grantedBlocks;
    }

    void setGrantedBlocks(const std::map<unsigned short, unsigned int>& rbMap)
    {
        grantedBlocks = rbMap;
    }

    void setGrantUpdate(bool update)
    {
        grantUpdate_ = update;
    }

    bool getGrantUpdate() const
    {
        return grantUpdate_;
    }

    void setGrantStop(bool stop)
    {
        grantStop_ = stop;
    }

    bool getGrantStop() const
    {
        return grantStop_;
    }

    void setNewGrant(bool newGrant)
    {
        newGrant_ = newGrant;
    }

    bool getNewGrant() const
    {
        return newGrant_;
    }

    void setPause(bool pause)
    {
        pause_ = pause;
    }

    bool getPause() const
    {
        return pause_;
    }
};

namespace omnetpp {

template<> inline Grant2Veh *fromAnyPtr(any_ptr ptr) { return check_and_cast<Grant2Veh*>(ptr.get<cObject>()); }

} // namespace omnetpp

#endif
