//
// Generated file, do not edit! Created by opp_msgtool 6.0 from mecrt/packets/apps/ServiceStatus.msg.
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
#include "ServiceStatus_m.h"

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

Register_Class(ServiceStatus)

ServiceStatus::ServiceStatus() : ::inet::FieldsChunk()
{
    this->setChunkLength(inet::B(34));

}

ServiceStatus::ServiceStatus(const ServiceStatus& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

ServiceStatus::~ServiceStatus()
{
}

ServiceStatus& ServiceStatus::operator=(const ServiceStatus& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void ServiceStatus::copy(const ServiceStatus& other)
{
    this->appId = other.appId;
    this->processGnbId = other.processGnbId;
    this->offloadGnbId = other.offloadGnbId;
    this->processGnbPort = other.processGnbPort;
    this->success = other.success;
    this->grantedBand = other.grantedBand;
    this->usedBand = other.usedBand;
    this->availBand = other.availBand;
    this->availCmpUnit = other.availCmpUnit;
    this->offloadGnbRbUpdateTime = other.offloadGnbRbUpdateTime;
    this->processGnbCuUpdateTime = other.processGnbCuUpdateTime;
}

void ServiceStatus::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->appId);
    doParsimPacking(b,this->processGnbId);
    doParsimPacking(b,this->offloadGnbId);
    doParsimPacking(b,this->processGnbPort);
    doParsimPacking(b,this->success);
    doParsimPacking(b,this->grantedBand);
    doParsimPacking(b,this->usedBand);
    doParsimPacking(b,this->availBand);
    doParsimPacking(b,this->availCmpUnit);
    doParsimPacking(b,this->offloadGnbRbUpdateTime);
    doParsimPacking(b,this->processGnbCuUpdateTime);
}

void ServiceStatus::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->appId);
    doParsimUnpacking(b,this->processGnbId);
    doParsimUnpacking(b,this->offloadGnbId);
    doParsimUnpacking(b,this->processGnbPort);
    doParsimUnpacking(b,this->success);
    doParsimUnpacking(b,this->grantedBand);
    doParsimUnpacking(b,this->usedBand);
    doParsimUnpacking(b,this->availBand);
    doParsimUnpacking(b,this->availCmpUnit);
    doParsimUnpacking(b,this->offloadGnbRbUpdateTime);
    doParsimUnpacking(b,this->processGnbCuUpdateTime);
}

unsigned int ServiceStatus::getAppId() const
{
    return this->appId;
}

void ServiceStatus::setAppId(unsigned int appId)
{
    handleChange();
    this->appId = appId;
}

unsigned short ServiceStatus::getProcessGnbId() const
{
    return this->processGnbId;
}

void ServiceStatus::setProcessGnbId(unsigned short processGnbId)
{
    handleChange();
    this->processGnbId = processGnbId;
}

unsigned short ServiceStatus::getOffloadGnbId() const
{
    return this->offloadGnbId;
}

void ServiceStatus::setOffloadGnbId(unsigned short offloadGnbId)
{
    handleChange();
    this->offloadGnbId = offloadGnbId;
}

int ServiceStatus::getProcessGnbPort() const
{
    return this->processGnbPort;
}

void ServiceStatus::setProcessGnbPort(int processGnbPort)
{
    handleChange();
    this->processGnbPort = processGnbPort;
}

bool ServiceStatus::getSuccess() const
{
    return this->success;
}

void ServiceStatus::setSuccess(bool success)
{
    handleChange();
    this->success = success;
}

int ServiceStatus::getGrantedBand() const
{
    return this->grantedBand;
}

void ServiceStatus::setGrantedBand(int grantedBand)
{
    handleChange();
    this->grantedBand = grantedBand;
}

int ServiceStatus::getUsedBand() const
{
    return this->usedBand;
}

void ServiceStatus::setUsedBand(int usedBand)
{
    handleChange();
    this->usedBand = usedBand;
}

int ServiceStatus::getAvailBand() const
{
    return this->availBand;
}

void ServiceStatus::setAvailBand(int availBand)
{
    handleChange();
    this->availBand = availBand;
}

int ServiceStatus::getAvailCmpUnit() const
{
    return this->availCmpUnit;
}

void ServiceStatus::setAvailCmpUnit(int availCmpUnit)
{
    handleChange();
    this->availCmpUnit = availCmpUnit;
}

omnetpp::simtime_t ServiceStatus::getOffloadGnbRbUpdateTime() const
{
    return this->offloadGnbRbUpdateTime;
}

void ServiceStatus::setOffloadGnbRbUpdateTime(omnetpp::simtime_t offloadGnbRbUpdateTime)
{
    handleChange();
    this->offloadGnbRbUpdateTime = offloadGnbRbUpdateTime;
}

omnetpp::simtime_t ServiceStatus::getProcessGnbCuUpdateTime() const
{
    return this->processGnbCuUpdateTime;
}

void ServiceStatus::setProcessGnbCuUpdateTime(omnetpp::simtime_t processGnbCuUpdateTime)
{
    handleChange();
    this->processGnbCuUpdateTime = processGnbCuUpdateTime;
}

class ServiceStatusDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_appId,
        FIELD_processGnbId,
        FIELD_offloadGnbId,
        FIELD_processGnbPort,
        FIELD_success,
        FIELD_grantedBand,
        FIELD_usedBand,
        FIELD_availBand,
        FIELD_availCmpUnit,
        FIELD_offloadGnbRbUpdateTime,
        FIELD_processGnbCuUpdateTime,
    };
  public:
    ServiceStatusDescriptor();
    virtual ~ServiceStatusDescriptor();

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

Register_ClassDescriptor(ServiceStatusDescriptor)

ServiceStatusDescriptor::ServiceStatusDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(ServiceStatus)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

ServiceStatusDescriptor::~ServiceStatusDescriptor()
{
    delete[] propertyNames;
}

bool ServiceStatusDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<ServiceStatus *>(obj)!=nullptr;
}

const char **ServiceStatusDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *ServiceStatusDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int ServiceStatusDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 11+base->getFieldCount() : 11;
}

unsigned int ServiceStatusDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_appId
        FD_ISEDITABLE,    // FIELD_processGnbId
        FD_ISEDITABLE,    // FIELD_offloadGnbId
        FD_ISEDITABLE,    // FIELD_processGnbPort
        FD_ISEDITABLE,    // FIELD_success
        FD_ISEDITABLE,    // FIELD_grantedBand
        FD_ISEDITABLE,    // FIELD_usedBand
        FD_ISEDITABLE,    // FIELD_availBand
        FD_ISEDITABLE,    // FIELD_availCmpUnit
        FD_ISEDITABLE,    // FIELD_offloadGnbRbUpdateTime
        FD_ISEDITABLE,    // FIELD_processGnbCuUpdateTime
    };
    return (field >= 0 && field < 11) ? fieldTypeFlags[field] : 0;
}

const char *ServiceStatusDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "appId",
        "processGnbId",
        "offloadGnbId",
        "processGnbPort",
        "success",
        "grantedBand",
        "usedBand",
        "availBand",
        "availCmpUnit",
        "offloadGnbRbUpdateTime",
        "processGnbCuUpdateTime",
    };
    return (field >= 0 && field < 11) ? fieldNames[field] : nullptr;
}

int ServiceStatusDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "appId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "processGnbId") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "offloadGnbId") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "processGnbPort") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "success") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "grantedBand") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "usedBand") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "availBand") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "availCmpUnit") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "offloadGnbRbUpdateTime") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "processGnbCuUpdateTime") == 0) return baseIndex + 10;
    return base ? base->findField(fieldName) : -1;
}

const char *ServiceStatusDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "unsigned int",    // FIELD_appId
        "unsigned short",    // FIELD_processGnbId
        "unsigned short",    // FIELD_offloadGnbId
        "int",    // FIELD_processGnbPort
        "bool",    // FIELD_success
        "int",    // FIELD_grantedBand
        "int",    // FIELD_usedBand
        "int",    // FIELD_availBand
        "int",    // FIELD_availCmpUnit
        "omnetpp::simtime_t",    // FIELD_offloadGnbRbUpdateTime
        "omnetpp::simtime_t",    // FIELD_processGnbCuUpdateTime
    };
    return (field >= 0 && field < 11) ? fieldTypeStrings[field] : nullptr;
}

const char **ServiceStatusDescriptor::getFieldPropertyNames(int field) const
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

const char *ServiceStatusDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int ServiceStatusDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    ServiceStatus *pp = omnetpp::fromAnyPtr<ServiceStatus>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void ServiceStatusDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    ServiceStatus *pp = omnetpp::fromAnyPtr<ServiceStatus>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'ServiceStatus'", field);
    }
}

const char *ServiceStatusDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    ServiceStatus *pp = omnetpp::fromAnyPtr<ServiceStatus>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string ServiceStatusDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    ServiceStatus *pp = omnetpp::fromAnyPtr<ServiceStatus>(object); (void)pp;
    switch (field) {
        case FIELD_appId: return ulong2string(pp->getAppId());
        case FIELD_processGnbId: return ulong2string(pp->getProcessGnbId());
        case FIELD_offloadGnbId: return ulong2string(pp->getOffloadGnbId());
        case FIELD_processGnbPort: return long2string(pp->getProcessGnbPort());
        case FIELD_success: return bool2string(pp->getSuccess());
        case FIELD_grantedBand: return long2string(pp->getGrantedBand());
        case FIELD_usedBand: return long2string(pp->getUsedBand());
        case FIELD_availBand: return long2string(pp->getAvailBand());
        case FIELD_availCmpUnit: return long2string(pp->getAvailCmpUnit());
        case FIELD_offloadGnbRbUpdateTime: return simtime2string(pp->getOffloadGnbRbUpdateTime());
        case FIELD_processGnbCuUpdateTime: return simtime2string(pp->getProcessGnbCuUpdateTime());
        default: return "";
    }
}

void ServiceStatusDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    ServiceStatus *pp = omnetpp::fromAnyPtr<ServiceStatus>(object); (void)pp;
    switch (field) {
        case FIELD_appId: pp->setAppId(string2ulong(value)); break;
        case FIELD_processGnbId: pp->setProcessGnbId(string2ulong(value)); break;
        case FIELD_offloadGnbId: pp->setOffloadGnbId(string2ulong(value)); break;
        case FIELD_processGnbPort: pp->setProcessGnbPort(string2long(value)); break;
        case FIELD_success: pp->setSuccess(string2bool(value)); break;
        case FIELD_grantedBand: pp->setGrantedBand(string2long(value)); break;
        case FIELD_usedBand: pp->setUsedBand(string2long(value)); break;
        case FIELD_availBand: pp->setAvailBand(string2long(value)); break;
        case FIELD_availCmpUnit: pp->setAvailCmpUnit(string2long(value)); break;
        case FIELD_offloadGnbRbUpdateTime: pp->setOffloadGnbRbUpdateTime(string2simtime(value)); break;
        case FIELD_processGnbCuUpdateTime: pp->setProcessGnbCuUpdateTime(string2simtime(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'ServiceStatus'", field);
    }
}

omnetpp::cValue ServiceStatusDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    ServiceStatus *pp = omnetpp::fromAnyPtr<ServiceStatus>(object); (void)pp;
    switch (field) {
        case FIELD_appId: return (omnetpp::intval_t)(pp->getAppId());
        case FIELD_processGnbId: return (omnetpp::intval_t)(pp->getProcessGnbId());
        case FIELD_offloadGnbId: return (omnetpp::intval_t)(pp->getOffloadGnbId());
        case FIELD_processGnbPort: return pp->getProcessGnbPort();
        case FIELD_success: return pp->getSuccess();
        case FIELD_grantedBand: return pp->getGrantedBand();
        case FIELD_usedBand: return pp->getUsedBand();
        case FIELD_availBand: return pp->getAvailBand();
        case FIELD_availCmpUnit: return pp->getAvailCmpUnit();
        case FIELD_offloadGnbRbUpdateTime: return pp->getOffloadGnbRbUpdateTime().dbl();
        case FIELD_processGnbCuUpdateTime: return pp->getProcessGnbCuUpdateTime().dbl();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'ServiceStatus' as cValue -- field index out of range?", field);
    }
}

void ServiceStatusDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    ServiceStatus *pp = omnetpp::fromAnyPtr<ServiceStatus>(object); (void)pp;
    switch (field) {
        case FIELD_appId: pp->setAppId(omnetpp::checked_int_cast<unsigned int>(value.intValue())); break;
        case FIELD_processGnbId: pp->setProcessGnbId(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_offloadGnbId: pp->setOffloadGnbId(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_processGnbPort: pp->setProcessGnbPort(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_success: pp->setSuccess(value.boolValue()); break;
        case FIELD_grantedBand: pp->setGrantedBand(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_usedBand: pp->setUsedBand(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_availBand: pp->setAvailBand(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_availCmpUnit: pp->setAvailCmpUnit(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_offloadGnbRbUpdateTime: pp->setOffloadGnbRbUpdateTime(value.doubleValue()); break;
        case FIELD_processGnbCuUpdateTime: pp->setProcessGnbCuUpdateTime(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'ServiceStatus'", field);
    }
}

const char *ServiceStatusDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr ServiceStatusDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    ServiceStatus *pp = omnetpp::fromAnyPtr<ServiceStatus>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void ServiceStatusDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    ServiceStatus *pp = omnetpp::fromAnyPtr<ServiceStatus>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'ServiceStatus'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

