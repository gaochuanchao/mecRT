//
// Generated file, do not edit! Created by opp_msgtool 6.0 from mecrt/packets/routing/OspfLsa.msg.
//

// Disable warnings about unused variables, empty switch stmts, etc:
#ifdef _MSC_VER
#  pragma warning(disable:4101)
#  pragma warning(disable:4065)
#endif

#if defined(__clang__)
#  pragma clang diagnostic ignored "-Wshadow"
#  pragma clang diagnostic ignored "-Wconversion"
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wc++98-compat"
#  pragma clang diagnostic ignored "-Wunreachable-code-break"
#  pragma clang diagnostic ignored "-Wold-style-cast"
#elif defined(__GNUC__)
#  pragma GCC diagnostic ignored "-Wshadow"
#  pragma GCC diagnostic ignored "-Wconversion"
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wold-style-cast"
#  pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
#  pragma GCC diagnostic ignored "-Wfloat-conversion"
#endif

#include <iostream>
#include <sstream>
#include <memory>
#include <type_traits>
#include "OspfLsa_m.h"

namespace omnetpp {

// Template pack/unpack rules. They are declared *after* a1l type-specific pack functions for multiple reasons.
// They are in the omnetpp namespace, to allow them to be found by argument-dependent lookup via the cCommBuffer argument

// Packing/unpacking an std::vector
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::vector<T,A>& v)
{
    int n = v.size();
    doParsimPacking(buffer, n);
    for (int i = 0; i < n; i++)
        doParsimPacking(buffer, v[i]);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::vector<T,A>& v)
{
    int n;
    doParsimUnpacking(buffer, n);
    v.resize(n);
    for (int i = 0; i < n; i++)
        doParsimUnpacking(buffer, v[i]);
}

// Packing/unpacking an std::list
template<typename T, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::list<T,A>& l)
{
    doParsimPacking(buffer, (int)l.size());
    for (typename std::list<T,A>::const_iterator it = l.begin(); it != l.end(); ++it)
        doParsimPacking(buffer, (T&)*it);
}

template<typename T, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::list<T,A>& l)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        l.push_back(T());
        doParsimUnpacking(buffer, l.back());
    }
}

// Packing/unpacking an std::set
template<typename T, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::set<T,Tr,A>& s)
{
    doParsimPacking(buffer, (int)s.size());
    for (typename std::set<T,Tr,A>::const_iterator it = s.begin(); it != s.end(); ++it)
        doParsimPacking(buffer, *it);
}

template<typename T, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::set<T,Tr,A>& s)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        T x;
        doParsimUnpacking(buffer, x);
        s.insert(x);
    }
}

// Packing/unpacking an std::map
template<typename K, typename V, typename Tr, typename A>
void doParsimPacking(omnetpp::cCommBuffer *buffer, const std::map<K,V,Tr,A>& m)
{
    doParsimPacking(buffer, (int)m.size());
    for (typename std::map<K,V,Tr,A>::const_iterator it = m.begin(); it != m.end(); ++it) {
        doParsimPacking(buffer, it->first);
        doParsimPacking(buffer, it->second);
    }
}

template<typename K, typename V, typename Tr, typename A>
void doParsimUnpacking(omnetpp::cCommBuffer *buffer, std::map<K,V,Tr,A>& m)
{
    int n;
    doParsimUnpacking(buffer, n);
    for (int i = 0; i < n; i++) {
        K k; V v;
        doParsimUnpacking(buffer, k);
        doParsimUnpacking(buffer, v);
        m[k] = v;
    }
}

// Default pack/unpack function for arrays
template<typename T>
void doParsimArrayPacking(omnetpp::cCommBuffer *b, const T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimPacking(b, t[i]);
}

template<typename T>
void doParsimArrayUnpacking(omnetpp::cCommBuffer *b, T *t, int n)
{
    for (int i = 0; i < n; i++)
        doParsimUnpacking(b, t[i]);
}

// Default rule to prevent compiler from choosing base class' doParsimPacking() function
template<typename T>
void doParsimPacking(omnetpp::cCommBuffer *, const T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimPacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

template<typename T>
void doParsimUnpacking(omnetpp::cCommBuffer *, T& t)
{
    throw omnetpp::cRuntimeError("Parsim error: No doParsimUnpacking() function for type %s", omnetpp::opp_typename(typeid(t)));
}

}  // namespace omnetpp

Register_Class(OspfLsa)

OspfLsa::OspfLsa() : ::inet::FieldsChunk()
{
    this->setChunkLength(inet::B(12));

}

OspfLsa::OspfLsa(const OspfLsa& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

OspfLsa::~OspfLsa()
{
    delete [] this->neighbor;
    delete [] this->cost;
}

OspfLsa& OspfLsa::operator=(const OspfLsa& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void OspfLsa::copy(const OspfLsa& other)
{
    this->origin = other.origin;
    this->seqNum = other.seqNum;
    this->installTime = other.installTime;
    delete [] this->neighbor;
    this->neighbor = (other.neighbor_arraysize==0) ? nullptr : new inet::Ipv4Address[other.neighbor_arraysize];
    neighbor_arraysize = other.neighbor_arraysize;
    for (size_t i = 0; i < neighbor_arraysize; i++) {
        this->neighbor[i] = other.neighbor[i];
    }
    delete [] this->cost;
    this->cost = (other.cost_arraysize==0) ? nullptr : new double[other.cost_arraysize];
    cost_arraysize = other.cost_arraysize;
    for (size_t i = 0; i < cost_arraysize; i++) {
        this->cost[i] = other.cost[i];
    }
}

void OspfLsa::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->origin);
    doParsimPacking(b,this->seqNum);
    doParsimPacking(b,this->installTime);
    b->pack(neighbor_arraysize);
    doParsimArrayPacking(b,this->neighbor,neighbor_arraysize);
    b->pack(cost_arraysize);
    doParsimArrayPacking(b,this->cost,cost_arraysize);
}

void OspfLsa::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->origin);
    doParsimUnpacking(b,this->seqNum);
    doParsimUnpacking(b,this->installTime);
    delete [] this->neighbor;
    b->unpack(neighbor_arraysize);
    if (neighbor_arraysize == 0) {
        this->neighbor = nullptr;
    } else {
        this->neighbor = new inet::Ipv4Address[neighbor_arraysize];
        doParsimArrayUnpacking(b,this->neighbor,neighbor_arraysize);
    }
    delete [] this->cost;
    b->unpack(cost_arraysize);
    if (cost_arraysize == 0) {
        this->cost = nullptr;
    } else {
        this->cost = new double[cost_arraysize];
        doParsimArrayUnpacking(b,this->cost,cost_arraysize);
    }
}

const inet::Ipv4Address& OspfLsa::getOrigin() const
{
    return this->origin;
}

void OspfLsa::setOrigin(const inet::Ipv4Address& origin)
{
    handleChange();
    this->origin = origin;
}

uint32_t OspfLsa::getSeqNum() const
{
    return this->seqNum;
}

void OspfLsa::setSeqNum(uint32_t seqNum)
{
    handleChange();
    this->seqNum = seqNum;
}

omnetpp::simtime_t OspfLsa::getInstallTime() const
{
    return this->installTime;
}

void OspfLsa::setInstallTime(omnetpp::simtime_t installTime)
{
    handleChange();
    this->installTime = installTime;
}

size_t OspfLsa::getNeighborArraySize() const
{
    return neighbor_arraysize;
}

const inet::Ipv4Address& OspfLsa::getNeighbor(size_t k) const
{
    if (k >= neighbor_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)neighbor_arraysize, (unsigned long)k);
    return this->neighbor[k];
}

void OspfLsa::setNeighborArraySize(size_t newSize)
{
    handleChange();
    inet::Ipv4Address *neighbor2 = (newSize==0) ? nullptr : new inet::Ipv4Address[newSize];
    size_t minSize = neighbor_arraysize < newSize ? neighbor_arraysize : newSize;
    for (size_t i = 0; i < minSize; i++)
        neighbor2[i] = this->neighbor[i];
    delete [] this->neighbor;
    this->neighbor = neighbor2;
    neighbor_arraysize = newSize;
}

void OspfLsa::setNeighbor(size_t k, const inet::Ipv4Address& neighbor)
{
    if (k >= neighbor_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)neighbor_arraysize, (unsigned long)k);
    handleChange();
    this->neighbor[k] = neighbor;
}

void OspfLsa::insertNeighbor(size_t k, const inet::Ipv4Address& neighbor)
{
    if (k > neighbor_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)neighbor_arraysize, (unsigned long)k);
    handleChange();
    size_t newSize = neighbor_arraysize + 1;
    inet::Ipv4Address *neighbor2 = new inet::Ipv4Address[newSize];
    size_t i;
    for (i = 0; i < k; i++)
        neighbor2[i] = this->neighbor[i];
    neighbor2[k] = neighbor;
    for (i = k + 1; i < newSize; i++)
        neighbor2[i] = this->neighbor[i-1];
    delete [] this->neighbor;
    this->neighbor = neighbor2;
    neighbor_arraysize = newSize;
}

void OspfLsa::appendNeighbor(const inet::Ipv4Address& neighbor)
{
    insertNeighbor(neighbor_arraysize, neighbor);
}

void OspfLsa::eraseNeighbor(size_t k)
{
    if (k >= neighbor_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)neighbor_arraysize, (unsigned long)k);
    handleChange();
    size_t newSize = neighbor_arraysize - 1;
    inet::Ipv4Address *neighbor2 = (newSize == 0) ? nullptr : new inet::Ipv4Address[newSize];
    size_t i;
    for (i = 0; i < k; i++)
        neighbor2[i] = this->neighbor[i];
    for (i = k; i < newSize; i++)
        neighbor2[i] = this->neighbor[i+1];
    delete [] this->neighbor;
    this->neighbor = neighbor2;
    neighbor_arraysize = newSize;
}

size_t OspfLsa::getCostArraySize() const
{
    return cost_arraysize;
}

double OspfLsa::getCost(size_t k) const
{
    if (k >= cost_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)cost_arraysize, (unsigned long)k);
    return this->cost[k];
}

void OspfLsa::setCostArraySize(size_t newSize)
{
    handleChange();
    double *cost2 = (newSize==0) ? nullptr : new double[newSize];
    size_t minSize = cost_arraysize < newSize ? cost_arraysize : newSize;
    for (size_t i = 0; i < minSize; i++)
        cost2[i] = this->cost[i];
    for (size_t i = minSize; i < newSize; i++)
        cost2[i] = 0;
    delete [] this->cost;
    this->cost = cost2;
    cost_arraysize = newSize;
}

void OspfLsa::setCost(size_t k, double cost)
{
    if (k >= cost_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)cost_arraysize, (unsigned long)k);
    handleChange();
    this->cost[k] = cost;
}

void OspfLsa::insertCost(size_t k, double cost)
{
    if (k > cost_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)cost_arraysize, (unsigned long)k);
    handleChange();
    size_t newSize = cost_arraysize + 1;
    double *cost2 = new double[newSize];
    size_t i;
    for (i = 0; i < k; i++)
        cost2[i] = this->cost[i];
    cost2[k] = cost;
    for (i = k + 1; i < newSize; i++)
        cost2[i] = this->cost[i-1];
    delete [] this->cost;
    this->cost = cost2;
    cost_arraysize = newSize;
}

void OspfLsa::appendCost(double cost)
{
    insertCost(cost_arraysize, cost);
}

void OspfLsa::eraseCost(size_t k)
{
    if (k >= cost_arraysize) throw omnetpp::cRuntimeError("Array of size %lu indexed by %lu", (unsigned long)cost_arraysize, (unsigned long)k);
    handleChange();
    size_t newSize = cost_arraysize - 1;
    double *cost2 = (newSize == 0) ? nullptr : new double[newSize];
    size_t i;
    for (i = 0; i < k; i++)
        cost2[i] = this->cost[i];
    for (i = k; i < newSize; i++)
        cost2[i] = this->cost[i+1];
    delete [] this->cost;
    this->cost = cost2;
    cost_arraysize = newSize;
}

class OspfLsaDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_origin,
        FIELD_seqNum,
        FIELD_installTime,
        FIELD_neighbor,
        FIELD_cost,
    };
  public:
    OspfLsaDescriptor();
    virtual ~OspfLsaDescriptor();

    virtual bool doesSupport(omnetpp::cObject *obj) const override;
    virtual const char **getPropertyNames() const override;
    virtual const char *getProperty(const char *propertyName) const override;
    virtual int getFieldCount() const override;
    virtual const char *getFieldName(int field) const override;
    virtual int findField(const char *fieldName) const override;
    virtual unsigned int getFieldTypeFlags(int field) const override;
    virtual const char *getFieldTypeString(int field) const override;
    virtual const char **getFieldPropertyNames(int field) const override;
    virtual const char *getFieldProperty(int field, const char *propertyName) const override;
    virtual int getFieldArraySize(omnetpp::any_ptr object, int field) const override;
    virtual void setFieldArraySize(omnetpp::any_ptr object, int field, int size) const override;

    virtual const char *getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const override;
    virtual std::string getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const override;
    virtual omnetpp::cValue getFieldValue(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const override;

    virtual const char *getFieldStructName(int field) const override;
    virtual omnetpp::any_ptr getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const override;
    virtual void setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const override;
};

Register_ClassDescriptor(OspfLsaDescriptor)

OspfLsaDescriptor::OspfLsaDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(OspfLsa)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

OspfLsaDescriptor::~OspfLsaDescriptor()
{
    delete[] propertyNames;
}

bool OspfLsaDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<OspfLsa *>(obj)!=nullptr;
}

const char **OspfLsaDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *OspfLsaDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int OspfLsaDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 5+base->getFieldCount() : 5;
}

unsigned int OspfLsaDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        0,    // FIELD_origin
        FD_ISEDITABLE,    // FIELD_seqNum
        FD_ISEDITABLE,    // FIELD_installTime
        FD_ISARRAY | FD_ISRESIZABLE,    // FIELD_neighbor
        FD_ISARRAY | FD_ISEDITABLE | FD_ISRESIZABLE,    // FIELD_cost
    };
    return (field >= 0 && field < 5) ? fieldTypeFlags[field] : 0;
}

const char *OspfLsaDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "origin",
        "seqNum",
        "installTime",
        "neighbor",
        "cost",
    };
    return (field >= 0 && field < 5) ? fieldNames[field] : nullptr;
}

int OspfLsaDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "origin") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "seqNum") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "installTime") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "neighbor") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "cost") == 0) return baseIndex + 4;
    return base ? base->findField(fieldName) : -1;
}

const char *OspfLsaDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "inet::Ipv4Address",    // FIELD_origin
        "uint32",    // FIELD_seqNum
        "omnetpp::simtime_t",    // FIELD_installTime
        "inet::Ipv4Address",    // FIELD_neighbor
        "double",    // FIELD_cost
    };
    return (field >= 0 && field < 5) ? fieldTypeStrings[field] : nullptr;
}

const char **OspfLsaDescriptor::getFieldPropertyNames(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldPropertyNames(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

const char *OspfLsaDescriptor::getFieldProperty(int field, const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldProperty(field, propertyName);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    }
}

int OspfLsaDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    OspfLsa *pp = omnetpp::fromAnyPtr<OspfLsa>(object); (void)pp;
    switch (field) {
        case FIELD_neighbor: return pp->getNeighborArraySize();
        case FIELD_cost: return pp->getCostArraySize();
        default: return 0;
    }
}

void OspfLsaDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    OspfLsa *pp = omnetpp::fromAnyPtr<OspfLsa>(object); (void)pp;
    switch (field) {
        case FIELD_neighbor: pp->setNeighborArraySize(size); break;
        case FIELD_cost: pp->setCostArraySize(size); break;
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'OspfLsa'", field);
    }
}

const char *OspfLsaDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    OspfLsa *pp = omnetpp::fromAnyPtr<OspfLsa>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string OspfLsaDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    OspfLsa *pp = omnetpp::fromAnyPtr<OspfLsa>(object); (void)pp;
    switch (field) {
        case FIELD_origin: return pp->getOrigin().str();
        case FIELD_seqNum: return ulong2string(pp->getSeqNum());
        case FIELD_installTime: return simtime2string(pp->getInstallTime());
        case FIELD_neighbor: return pp->getNeighbor(i).str();
        case FIELD_cost: return double2string(pp->getCost(i));
        default: return "";
    }
}

void OspfLsaDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    OspfLsa *pp = omnetpp::fromAnyPtr<OspfLsa>(object); (void)pp;
    switch (field) {
        case FIELD_seqNum: pp->setSeqNum(string2ulong(value)); break;
        case FIELD_installTime: pp->setInstallTime(string2simtime(value)); break;
        case FIELD_cost: pp->setCost(i,string2double(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'OspfLsa'", field);
    }
}

omnetpp::cValue OspfLsaDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    OspfLsa *pp = omnetpp::fromAnyPtr<OspfLsa>(object); (void)pp;
    switch (field) {
        case FIELD_origin: return omnetpp::toAnyPtr(&pp->getOrigin()); break;
        case FIELD_seqNum: return (omnetpp::intval_t)(pp->getSeqNum());
        case FIELD_installTime: return pp->getInstallTime().dbl();
        case FIELD_neighbor: return omnetpp::toAnyPtr(&pp->getNeighbor(i)); break;
        case FIELD_cost: return pp->getCost(i);
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'OspfLsa' as cValue -- field index out of range?", field);
    }
}

void OspfLsaDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    OspfLsa *pp = omnetpp::fromAnyPtr<OspfLsa>(object); (void)pp;
    switch (field) {
        case FIELD_seqNum: pp->setSeqNum(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_installTime: pp->setInstallTime(value.doubleValue()); break;
        case FIELD_cost: pp->setCost(i,value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'OspfLsa'", field);
    }
}

const char *OspfLsaDescriptor::getFieldStructName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructName(field);
        field -= base->getFieldCount();
    }
    switch (field) {
        default: return nullptr;
    };
}

omnetpp::any_ptr OspfLsaDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    OspfLsa *pp = omnetpp::fromAnyPtr<OspfLsa>(object); (void)pp;
    switch (field) {
        case FIELD_origin: return omnetpp::toAnyPtr(&pp->getOrigin()); break;
        case FIELD_neighbor: return omnetpp::toAnyPtr(&pp->getNeighbor(i)); break;
        default: return omnetpp::any_ptr(nullptr);
    }
}

void OspfLsaDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    OspfLsa *pp = omnetpp::fromAnyPtr<OspfLsa>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'OspfLsa'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

