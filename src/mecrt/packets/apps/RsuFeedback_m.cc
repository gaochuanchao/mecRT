//
// Generated file, do not edit! Created by opp_msgtool 6.0 from mecrt/packets/apps/RsuFeedback.msg.
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
#include "RsuFeedback_m.h"

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

Register_Class(RsuFeedback)

RsuFeedback::RsuFeedback() : ::inet::FieldsChunk()
{
    this->setChunkLength(inet::B(44));

}

RsuFeedback::RsuFeedback(const RsuFeedback& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

RsuFeedback::~RsuFeedback()
{
}

RsuFeedback& RsuFeedback::operator=(const RsuFeedback& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void RsuFeedback::copy(const RsuFeedback& other)
{
    this->gnbId = other.gnbId;
    this->vehId = other.vehId;
    this->serverPort = other.serverPort;
    this->frequency = other.frequency;
    this->bytePerBand = other.bytePerBand;
    this->availBands = other.availBands;
    this->totalBands = other.totalBands;
    this->freeCmpUnits = other.freeCmpUnits;
    this->totalCmpUnits = other.totalCmpUnits;
    this->deviceType = other.deviceType;
    this->resourceType = other.resourceType;
    this->bandUpdateTime = other.bandUpdateTime;
    this->cmpUnitUpdateTime = other.cmpUnitUpdateTime;
}

void RsuFeedback::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->gnbId);
    doParsimPacking(b,this->vehId);
    doParsimPacking(b,this->serverPort);
    doParsimPacking(b,this->frequency);
    doParsimPacking(b,this->bytePerBand);
    doParsimPacking(b,this->availBands);
    doParsimPacking(b,this->totalBands);
    doParsimPacking(b,this->freeCmpUnits);
    doParsimPacking(b,this->totalCmpUnits);
    doParsimPacking(b,this->deviceType);
    doParsimPacking(b,this->resourceType);
    doParsimPacking(b,this->bandUpdateTime);
    doParsimPacking(b,this->cmpUnitUpdateTime);
}

void RsuFeedback::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->gnbId);
    doParsimUnpacking(b,this->vehId);
    doParsimUnpacking(b,this->serverPort);
    doParsimUnpacking(b,this->frequency);
    doParsimUnpacking(b,this->bytePerBand);
    doParsimUnpacking(b,this->availBands);
    doParsimUnpacking(b,this->totalBands);
    doParsimUnpacking(b,this->freeCmpUnits);
    doParsimUnpacking(b,this->totalCmpUnits);
    doParsimUnpacking(b,this->deviceType);
    doParsimUnpacking(b,this->resourceType);
    doParsimUnpacking(b,this->bandUpdateTime);
    doParsimUnpacking(b,this->cmpUnitUpdateTime);
}

int RsuFeedback::getGnbId() const
{
    return this->gnbId;
}

void RsuFeedback::setGnbId(int gnbId)
{
    handleChange();
    this->gnbId = gnbId;
}

int RsuFeedback::getVehId() const
{
    return this->vehId;
}

void RsuFeedback::setVehId(int vehId)
{
    handleChange();
    this->vehId = vehId;
}

int RsuFeedback::getServerPort() const
{
    return this->serverPort;
}

void RsuFeedback::setServerPort(int serverPort)
{
    handleChange();
    this->serverPort = serverPort;
}

double RsuFeedback::getFrequency() const
{
    return this->frequency;
}

void RsuFeedback::setFrequency(double frequency)
{
    handleChange();
    this->frequency = frequency;
}

int RsuFeedback::getBytePerBand() const
{
    return this->bytePerBand;
}

void RsuFeedback::setBytePerBand(int bytePerBand)
{
    handleChange();
    this->bytePerBand = bytePerBand;
}

int RsuFeedback::getAvailBands() const
{
    return this->availBands;
}

void RsuFeedback::setAvailBands(int availBands)
{
    handleChange();
    this->availBands = availBands;
}

int RsuFeedback::getTotalBands() const
{
    return this->totalBands;
}

void RsuFeedback::setTotalBands(int totalBands)
{
    handleChange();
    this->totalBands = totalBands;
}

int RsuFeedback::getFreeCmpUnits() const
{
    return this->freeCmpUnits;
}

void RsuFeedback::setFreeCmpUnits(int freeCmpUnits)
{
    handleChange();
    this->freeCmpUnits = freeCmpUnits;
}

int RsuFeedback::getTotalCmpUnits() const
{
    return this->totalCmpUnits;
}

void RsuFeedback::setTotalCmpUnits(int totalCmpUnits)
{
    handleChange();
    this->totalCmpUnits = totalCmpUnits;
}

unsigned short RsuFeedback::getDeviceType() const
{
    return this->deviceType;
}

void RsuFeedback::setDeviceType(unsigned short deviceType)
{
    handleChange();
    this->deviceType = deviceType;
}

unsigned short RsuFeedback::getResourceType() const
{
    return this->resourceType;
}

void RsuFeedback::setResourceType(unsigned short resourceType)
{
    handleChange();
    this->resourceType = resourceType;
}

omnetpp::simtime_t RsuFeedback::getBandUpdateTime() const
{
    return this->bandUpdateTime;
}

void RsuFeedback::setBandUpdateTime(omnetpp::simtime_t bandUpdateTime)
{
    handleChange();
    this->bandUpdateTime = bandUpdateTime;
}

omnetpp::simtime_t RsuFeedback::getCmpUnitUpdateTime() const
{
    return this->cmpUnitUpdateTime;
}

void RsuFeedback::setCmpUnitUpdateTime(omnetpp::simtime_t cmpUnitUpdateTime)
{
    handleChange();
    this->cmpUnitUpdateTime = cmpUnitUpdateTime;
}

class RsuFeedbackDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_gnbId,
        FIELD_vehId,
        FIELD_serverPort,
        FIELD_frequency,
        FIELD_bytePerBand,
        FIELD_availBands,
        FIELD_totalBands,
        FIELD_freeCmpUnits,
        FIELD_totalCmpUnits,
        FIELD_deviceType,
        FIELD_resourceType,
        FIELD_bandUpdateTime,
        FIELD_cmpUnitUpdateTime,
    };
  public:
    RsuFeedbackDescriptor();
    virtual ~RsuFeedbackDescriptor();

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

Register_ClassDescriptor(RsuFeedbackDescriptor)

RsuFeedbackDescriptor::RsuFeedbackDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(RsuFeedback)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

RsuFeedbackDescriptor::~RsuFeedbackDescriptor()
{
    delete[] propertyNames;
}

bool RsuFeedbackDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<RsuFeedback *>(obj)!=nullptr;
}

const char **RsuFeedbackDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *RsuFeedbackDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int RsuFeedbackDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 13+base->getFieldCount() : 13;
}

unsigned int RsuFeedbackDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_gnbId
        FD_ISEDITABLE,    // FIELD_vehId
        FD_ISEDITABLE,    // FIELD_serverPort
        FD_ISEDITABLE,    // FIELD_frequency
        FD_ISEDITABLE,    // FIELD_bytePerBand
        FD_ISEDITABLE,    // FIELD_availBands
        FD_ISEDITABLE,    // FIELD_totalBands
        FD_ISEDITABLE,    // FIELD_freeCmpUnits
        FD_ISEDITABLE,    // FIELD_totalCmpUnits
        FD_ISEDITABLE,    // FIELD_deviceType
        FD_ISEDITABLE,    // FIELD_resourceType
        FD_ISEDITABLE,    // FIELD_bandUpdateTime
        FD_ISEDITABLE,    // FIELD_cmpUnitUpdateTime
    };
    return (field >= 0 && field < 13) ? fieldTypeFlags[field] : 0;
}

const char *RsuFeedbackDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "gnbId",
        "vehId",
        "serverPort",
        "frequency",
        "bytePerBand",
        "availBands",
        "totalBands",
        "freeCmpUnits",
        "totalCmpUnits",
        "deviceType",
        "resourceType",
        "bandUpdateTime",
        "cmpUnitUpdateTime",
    };
    return (field >= 0 && field < 13) ? fieldNames[field] : nullptr;
}

int RsuFeedbackDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "gnbId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "vehId") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "serverPort") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "frequency") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "bytePerBand") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "availBands") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "totalBands") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "freeCmpUnits") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "totalCmpUnits") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "deviceType") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "resourceType") == 0) return baseIndex + 10;
    if (strcmp(fieldName, "bandUpdateTime") == 0) return baseIndex + 11;
    if (strcmp(fieldName, "cmpUnitUpdateTime") == 0) return baseIndex + 12;
    return base ? base->findField(fieldName) : -1;
}

const char *RsuFeedbackDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_gnbId
        "int",    // FIELD_vehId
        "int",    // FIELD_serverPort
        "double",    // FIELD_frequency
        "int",    // FIELD_bytePerBand
        "int",    // FIELD_availBands
        "int",    // FIELD_totalBands
        "int",    // FIELD_freeCmpUnits
        "int",    // FIELD_totalCmpUnits
        "unsigned short",    // FIELD_deviceType
        "unsigned short",    // FIELD_resourceType
        "omnetpp::simtime_t",    // FIELD_bandUpdateTime
        "omnetpp::simtime_t",    // FIELD_cmpUnitUpdateTime
    };
    return (field >= 0 && field < 13) ? fieldTypeStrings[field] : nullptr;
}

const char **RsuFeedbackDescriptor::getFieldPropertyNames(int field) const
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

const char *RsuFeedbackDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int RsuFeedbackDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    RsuFeedback *pp = omnetpp::fromAnyPtr<RsuFeedback>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void RsuFeedbackDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    RsuFeedback *pp = omnetpp::fromAnyPtr<RsuFeedback>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'RsuFeedback'", field);
    }
}

const char *RsuFeedbackDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    RsuFeedback *pp = omnetpp::fromAnyPtr<RsuFeedback>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string RsuFeedbackDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    RsuFeedback *pp = omnetpp::fromAnyPtr<RsuFeedback>(object); (void)pp;
    switch (field) {
        case FIELD_gnbId: return long2string(pp->getGnbId());
        case FIELD_vehId: return long2string(pp->getVehId());
        case FIELD_serverPort: return long2string(pp->getServerPort());
        case FIELD_frequency: return double2string(pp->getFrequency());
        case FIELD_bytePerBand: return long2string(pp->getBytePerBand());
        case FIELD_availBands: return long2string(pp->getAvailBands());
        case FIELD_totalBands: return long2string(pp->getTotalBands());
        case FIELD_freeCmpUnits: return long2string(pp->getFreeCmpUnits());
        case FIELD_totalCmpUnits: return long2string(pp->getTotalCmpUnits());
        case FIELD_deviceType: return ulong2string(pp->getDeviceType());
        case FIELD_resourceType: return ulong2string(pp->getResourceType());
        case FIELD_bandUpdateTime: return simtime2string(pp->getBandUpdateTime());
        case FIELD_cmpUnitUpdateTime: return simtime2string(pp->getCmpUnitUpdateTime());
        default: return "";
    }
}

void RsuFeedbackDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    RsuFeedback *pp = omnetpp::fromAnyPtr<RsuFeedback>(object); (void)pp;
    switch (field) {
        case FIELD_gnbId: pp->setGnbId(string2long(value)); break;
        case FIELD_vehId: pp->setVehId(string2long(value)); break;
        case FIELD_serverPort: pp->setServerPort(string2long(value)); break;
        case FIELD_frequency: pp->setFrequency(string2double(value)); break;
        case FIELD_bytePerBand: pp->setBytePerBand(string2long(value)); break;
        case FIELD_availBands: pp->setAvailBands(string2long(value)); break;
        case FIELD_totalBands: pp->setTotalBands(string2long(value)); break;
        case FIELD_freeCmpUnits: pp->setFreeCmpUnits(string2long(value)); break;
        case FIELD_totalCmpUnits: pp->setTotalCmpUnits(string2long(value)); break;
        case FIELD_deviceType: pp->setDeviceType(string2ulong(value)); break;
        case FIELD_resourceType: pp->setResourceType(string2ulong(value)); break;
        case FIELD_bandUpdateTime: pp->setBandUpdateTime(string2simtime(value)); break;
        case FIELD_cmpUnitUpdateTime: pp->setCmpUnitUpdateTime(string2simtime(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RsuFeedback'", field);
    }
}

omnetpp::cValue RsuFeedbackDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    RsuFeedback *pp = omnetpp::fromAnyPtr<RsuFeedback>(object); (void)pp;
    switch (field) {
        case FIELD_gnbId: return pp->getGnbId();
        case FIELD_vehId: return pp->getVehId();
        case FIELD_serverPort: return pp->getServerPort();
        case FIELD_frequency: return pp->getFrequency();
        case FIELD_bytePerBand: return pp->getBytePerBand();
        case FIELD_availBands: return pp->getAvailBands();
        case FIELD_totalBands: return pp->getTotalBands();
        case FIELD_freeCmpUnits: return pp->getFreeCmpUnits();
        case FIELD_totalCmpUnits: return pp->getTotalCmpUnits();
        case FIELD_deviceType: return (omnetpp::intval_t)(pp->getDeviceType());
        case FIELD_resourceType: return (omnetpp::intval_t)(pp->getResourceType());
        case FIELD_bandUpdateTime: return pp->getBandUpdateTime().dbl();
        case FIELD_cmpUnitUpdateTime: return pp->getCmpUnitUpdateTime().dbl();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'RsuFeedback' as cValue -- field index out of range?", field);
    }
}

void RsuFeedbackDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    RsuFeedback *pp = omnetpp::fromAnyPtr<RsuFeedback>(object); (void)pp;
    switch (field) {
        case FIELD_gnbId: pp->setGnbId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_vehId: pp->setVehId(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_serverPort: pp->setServerPort(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_frequency: pp->setFrequency(value.doubleValue()); break;
        case FIELD_bytePerBand: pp->setBytePerBand(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_availBands: pp->setAvailBands(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_totalBands: pp->setTotalBands(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_freeCmpUnits: pp->setFreeCmpUnits(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_totalCmpUnits: pp->setTotalCmpUnits(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_deviceType: pp->setDeviceType(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_resourceType: pp->setResourceType(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_bandUpdateTime: pp->setBandUpdateTime(value.doubleValue()); break;
        case FIELD_cmpUnitUpdateTime: pp->setCmpUnitUpdateTime(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RsuFeedback'", field);
    }
}

const char *RsuFeedbackDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr RsuFeedbackDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    RsuFeedback *pp = omnetpp::fromAnyPtr<RsuFeedback>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void RsuFeedbackDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    RsuFeedback *pp = omnetpp::fromAnyPtr<RsuFeedback>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'RsuFeedback'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

