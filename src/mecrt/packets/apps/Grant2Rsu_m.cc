//
// Generated file, do not edit! Created by opp_msgtool 6.0 from mecrt/packets/apps/Grant2Rsu.msg.
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
#include "Grant2Rsu_m.h"

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

Register_Class(Grant2Rsu)

Grant2Rsu::Grant2Rsu() : ::inet::FieldsChunk()
{
    this->setChunkLength(inet::B(66));

}

Grant2Rsu::Grant2Rsu(const Grant2Rsu& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

Grant2Rsu::~Grant2Rsu()
{
}

Grant2Rsu& Grant2Rsu::operator=(const Grant2Rsu& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void Grant2Rsu::copy(const Grant2Rsu& other)
{
    this->appId = other.appId;
    this->ueAddr = other.ueAddr;
    this->resourceType = other.resourceType;
    this->service = other.service;
    this->processGnbId = other.processGnbId;
    this->offloadGnbId = other.offloadGnbId;
    this->offloadGnbAddr = other.offloadGnbAddr;
    this->cmpUnits = other.cmpUnits;
    this->bands = other.bands;
    this->exeTime = other.exeTime;
    this->maxOffloadTime = other.maxOffloadTime;
    this->deadline = other.deadline;
    this->outputSize = other.outputSize;
    this->inputSize = other.inputSize;
    this->start = other.start;
    this->stop = other.stop;
}

void Grant2Rsu::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->appId);
    doParsimPacking(b,this->ueAddr);
    doParsimPacking(b,this->resourceType);
    doParsimPacking(b,this->service);
    doParsimPacking(b,this->processGnbId);
    doParsimPacking(b,this->offloadGnbId);
    doParsimPacking(b,this->offloadGnbAddr);
    doParsimPacking(b,this->cmpUnits);
    doParsimPacking(b,this->bands);
    doParsimPacking(b,this->exeTime);
    doParsimPacking(b,this->maxOffloadTime);
    doParsimPacking(b,this->deadline);
    doParsimPacking(b,this->outputSize);
    doParsimPacking(b,this->inputSize);
    doParsimPacking(b,this->start);
    doParsimPacking(b,this->stop);
}

void Grant2Rsu::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->appId);
    doParsimUnpacking(b,this->ueAddr);
    doParsimUnpacking(b,this->resourceType);
    doParsimUnpacking(b,this->service);
    doParsimUnpacking(b,this->processGnbId);
    doParsimUnpacking(b,this->offloadGnbId);
    doParsimUnpacking(b,this->offloadGnbAddr);
    doParsimUnpacking(b,this->cmpUnits);
    doParsimUnpacking(b,this->bands);
    doParsimUnpacking(b,this->exeTime);
    doParsimUnpacking(b,this->maxOffloadTime);
    doParsimUnpacking(b,this->deadline);
    doParsimUnpacking(b,this->outputSize);
    doParsimUnpacking(b,this->inputSize);
    doParsimUnpacking(b,this->start);
    doParsimUnpacking(b,this->stop);
}

unsigned int Grant2Rsu::getAppId() const
{
    return this->appId;
}

void Grant2Rsu::setAppId(unsigned int appId)
{
    handleChange();
    this->appId = appId;
}

uint32_t Grant2Rsu::getUeAddr() const
{
    return this->ueAddr;
}

void Grant2Rsu::setUeAddr(uint32_t ueAddr)
{
    handleChange();
    this->ueAddr = ueAddr;
}

const char * Grant2Rsu::getResourceType() const
{
    return this->resourceType.c_str();
}

void Grant2Rsu::setResourceType(const char * resourceType)
{
    handleChange();
    this->resourceType = resourceType;
}

const char * Grant2Rsu::getService() const
{
    return this->service.c_str();
}

void Grant2Rsu::setService(const char * service)
{
    handleChange();
    this->service = service;
}

unsigned short Grant2Rsu::getProcessGnbId() const
{
    return this->processGnbId;
}

void Grant2Rsu::setProcessGnbId(unsigned short processGnbId)
{
    handleChange();
    this->processGnbId = processGnbId;
}

unsigned short Grant2Rsu::getOffloadGnbId() const
{
    return this->offloadGnbId;
}

void Grant2Rsu::setOffloadGnbId(unsigned short offloadGnbId)
{
    handleChange();
    this->offloadGnbId = offloadGnbId;
}

uint32_t Grant2Rsu::getOffloadGnbAddr() const
{
    return this->offloadGnbAddr;
}

void Grant2Rsu::setOffloadGnbAddr(uint32_t offloadGnbAddr)
{
    handleChange();
    this->offloadGnbAddr = offloadGnbAddr;
}

int Grant2Rsu::getCmpUnits() const
{
    return this->cmpUnits;
}

void Grant2Rsu::setCmpUnits(int cmpUnits)
{
    handleChange();
    this->cmpUnits = cmpUnits;
}

int Grant2Rsu::getBands() const
{
    return this->bands;
}

void Grant2Rsu::setBands(int bands)
{
    handleChange();
    this->bands = bands;
}

omnetpp::simtime_t Grant2Rsu::getExeTime() const
{
    return this->exeTime;
}

void Grant2Rsu::setExeTime(omnetpp::simtime_t exeTime)
{
    handleChange();
    this->exeTime = exeTime;
}

omnetpp::simtime_t Grant2Rsu::getMaxOffloadTime() const
{
    return this->maxOffloadTime;
}

void Grant2Rsu::setMaxOffloadTime(omnetpp::simtime_t maxOffloadTime)
{
    handleChange();
    this->maxOffloadTime = maxOffloadTime;
}

omnetpp::simtime_t Grant2Rsu::getDeadline() const
{
    return this->deadline;
}

void Grant2Rsu::setDeadline(omnetpp::simtime_t deadline)
{
    handleChange();
    this->deadline = deadline;
}

int Grant2Rsu::getOutputSize() const
{
    return this->outputSize;
}

void Grant2Rsu::setOutputSize(int outputSize)
{
    handleChange();
    this->outputSize = outputSize;
}

int Grant2Rsu::getInputSize() const
{
    return this->inputSize;
}

void Grant2Rsu::setInputSize(int inputSize)
{
    handleChange();
    this->inputSize = inputSize;
}

bool Grant2Rsu::getStart() const
{
    return this->start;
}

void Grant2Rsu::setStart(bool start)
{
    handleChange();
    this->start = start;
}

bool Grant2Rsu::getStop() const
{
    return this->stop;
}

void Grant2Rsu::setStop(bool stop)
{
    handleChange();
    this->stop = stop;
}

class Grant2RsuDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_appId,
        FIELD_ueAddr,
        FIELD_resourceType,
        FIELD_service,
        FIELD_processGnbId,
        FIELD_offloadGnbId,
        FIELD_offloadGnbAddr,
        FIELD_cmpUnits,
        FIELD_bands,
        FIELD_exeTime,
        FIELD_maxOffloadTime,
        FIELD_deadline,
        FIELD_outputSize,
        FIELD_inputSize,
        FIELD_start,
        FIELD_stop,
    };
  public:
    Grant2RsuDescriptor();
    virtual ~Grant2RsuDescriptor();

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

Register_ClassDescriptor(Grant2RsuDescriptor)

Grant2RsuDescriptor::Grant2RsuDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(Grant2Rsu)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

Grant2RsuDescriptor::~Grant2RsuDescriptor()
{
    delete[] propertyNames;
}

bool Grant2RsuDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<Grant2Rsu *>(obj)!=nullptr;
}

const char **Grant2RsuDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *Grant2RsuDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int Grant2RsuDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 16+base->getFieldCount() : 16;
}

unsigned int Grant2RsuDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_appId
        FD_ISEDITABLE,    // FIELD_ueAddr
        FD_ISEDITABLE,    // FIELD_resourceType
        FD_ISEDITABLE,    // FIELD_service
        FD_ISEDITABLE,    // FIELD_processGnbId
        FD_ISEDITABLE,    // FIELD_offloadGnbId
        FD_ISEDITABLE,    // FIELD_offloadGnbAddr
        FD_ISEDITABLE,    // FIELD_cmpUnits
        FD_ISEDITABLE,    // FIELD_bands
        FD_ISEDITABLE,    // FIELD_exeTime
        FD_ISEDITABLE,    // FIELD_maxOffloadTime
        FD_ISEDITABLE,    // FIELD_deadline
        FD_ISEDITABLE,    // FIELD_outputSize
        FD_ISEDITABLE,    // FIELD_inputSize
        FD_ISEDITABLE,    // FIELD_start
        FD_ISEDITABLE,    // FIELD_stop
    };
    return (field >= 0 && field < 16) ? fieldTypeFlags[field] : 0;
}

const char *Grant2RsuDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "appId",
        "ueAddr",
        "resourceType",
        "service",
        "processGnbId",
        "offloadGnbId",
        "offloadGnbAddr",
        "cmpUnits",
        "bands",
        "exeTime",
        "maxOffloadTime",
        "deadline",
        "outputSize",
        "inputSize",
        "start",
        "stop",
    };
    return (field >= 0 && field < 16) ? fieldNames[field] : nullptr;
}

int Grant2RsuDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "appId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "ueAddr") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "resourceType") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "service") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "processGnbId") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "offloadGnbId") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "offloadGnbAddr") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "cmpUnits") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "bands") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "exeTime") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "maxOffloadTime") == 0) return baseIndex + 10;
    if (strcmp(fieldName, "deadline") == 0) return baseIndex + 11;
    if (strcmp(fieldName, "outputSize") == 0) return baseIndex + 12;
    if (strcmp(fieldName, "inputSize") == 0) return baseIndex + 13;
    if (strcmp(fieldName, "start") == 0) return baseIndex + 14;
    if (strcmp(fieldName, "stop") == 0) return baseIndex + 15;
    return base ? base->findField(fieldName) : -1;
}

const char *Grant2RsuDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "unsigned int",    // FIELD_appId
        "uint32",    // FIELD_ueAddr
        "string",    // FIELD_resourceType
        "string",    // FIELD_service
        "unsigned short",    // FIELD_processGnbId
        "unsigned short",    // FIELD_offloadGnbId
        "uint32",    // FIELD_offloadGnbAddr
        "int",    // FIELD_cmpUnits
        "int",    // FIELD_bands
        "omnetpp::simtime_t",    // FIELD_exeTime
        "omnetpp::simtime_t",    // FIELD_maxOffloadTime
        "omnetpp::simtime_t",    // FIELD_deadline
        "int",    // FIELD_outputSize
        "int",    // FIELD_inputSize
        "bool",    // FIELD_start
        "bool",    // FIELD_stop
    };
    return (field >= 0 && field < 16) ? fieldTypeStrings[field] : nullptr;
}

const char **Grant2RsuDescriptor::getFieldPropertyNames(int field) const
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

const char *Grant2RsuDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int Grant2RsuDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    Grant2Rsu *pp = omnetpp::fromAnyPtr<Grant2Rsu>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void Grant2RsuDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    Grant2Rsu *pp = omnetpp::fromAnyPtr<Grant2Rsu>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'Grant2Rsu'", field);
    }
}

const char *Grant2RsuDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    Grant2Rsu *pp = omnetpp::fromAnyPtr<Grant2Rsu>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string Grant2RsuDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    Grant2Rsu *pp = omnetpp::fromAnyPtr<Grant2Rsu>(object); (void)pp;
    switch (field) {
        case FIELD_appId: return ulong2string(pp->getAppId());
        case FIELD_ueAddr: return ulong2string(pp->getUeAddr());
        case FIELD_resourceType: return oppstring2string(pp->getResourceType());
        case FIELD_service: return oppstring2string(pp->getService());
        case FIELD_processGnbId: return ulong2string(pp->getProcessGnbId());
        case FIELD_offloadGnbId: return ulong2string(pp->getOffloadGnbId());
        case FIELD_offloadGnbAddr: return ulong2string(pp->getOffloadGnbAddr());
        case FIELD_cmpUnits: return long2string(pp->getCmpUnits());
        case FIELD_bands: return long2string(pp->getBands());
        case FIELD_exeTime: return simtime2string(pp->getExeTime());
        case FIELD_maxOffloadTime: return simtime2string(pp->getMaxOffloadTime());
        case FIELD_deadline: return simtime2string(pp->getDeadline());
        case FIELD_outputSize: return long2string(pp->getOutputSize());
        case FIELD_inputSize: return long2string(pp->getInputSize());
        case FIELD_start: return bool2string(pp->getStart());
        case FIELD_stop: return bool2string(pp->getStop());
        default: return "";
    }
}

void Grant2RsuDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Grant2Rsu *pp = omnetpp::fromAnyPtr<Grant2Rsu>(object); (void)pp;
    switch (field) {
        case FIELD_appId: pp->setAppId(string2ulong(value)); break;
        case FIELD_ueAddr: pp->setUeAddr(string2ulong(value)); break;
        case FIELD_resourceType: pp->setResourceType((value)); break;
        case FIELD_service: pp->setService((value)); break;
        case FIELD_processGnbId: pp->setProcessGnbId(string2ulong(value)); break;
        case FIELD_offloadGnbId: pp->setOffloadGnbId(string2ulong(value)); break;
        case FIELD_offloadGnbAddr: pp->setOffloadGnbAddr(string2ulong(value)); break;
        case FIELD_cmpUnits: pp->setCmpUnits(string2long(value)); break;
        case FIELD_bands: pp->setBands(string2long(value)); break;
        case FIELD_exeTime: pp->setExeTime(string2simtime(value)); break;
        case FIELD_maxOffloadTime: pp->setMaxOffloadTime(string2simtime(value)); break;
        case FIELD_deadline: pp->setDeadline(string2simtime(value)); break;
        case FIELD_outputSize: pp->setOutputSize(string2long(value)); break;
        case FIELD_inputSize: pp->setInputSize(string2long(value)); break;
        case FIELD_start: pp->setStart(string2bool(value)); break;
        case FIELD_stop: pp->setStop(string2bool(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Grant2Rsu'", field);
    }
}

omnetpp::cValue Grant2RsuDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    Grant2Rsu *pp = omnetpp::fromAnyPtr<Grant2Rsu>(object); (void)pp;
    switch (field) {
        case FIELD_appId: return (omnetpp::intval_t)(pp->getAppId());
        case FIELD_ueAddr: return (omnetpp::intval_t)(pp->getUeAddr());
        case FIELD_resourceType: return pp->getResourceType();
        case FIELD_service: return pp->getService();
        case FIELD_processGnbId: return (omnetpp::intval_t)(pp->getProcessGnbId());
        case FIELD_offloadGnbId: return (omnetpp::intval_t)(pp->getOffloadGnbId());
        case FIELD_offloadGnbAddr: return (omnetpp::intval_t)(pp->getOffloadGnbAddr());
        case FIELD_cmpUnits: return pp->getCmpUnits();
        case FIELD_bands: return pp->getBands();
        case FIELD_exeTime: return pp->getExeTime().dbl();
        case FIELD_maxOffloadTime: return pp->getMaxOffloadTime().dbl();
        case FIELD_deadline: return pp->getDeadline().dbl();
        case FIELD_outputSize: return pp->getOutputSize();
        case FIELD_inputSize: return pp->getInputSize();
        case FIELD_start: return pp->getStart();
        case FIELD_stop: return pp->getStop();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'Grant2Rsu' as cValue -- field index out of range?", field);
    }
}

void Grant2RsuDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Grant2Rsu *pp = omnetpp::fromAnyPtr<Grant2Rsu>(object); (void)pp;
    switch (field) {
        case FIELD_appId: pp->setAppId(omnetpp::checked_int_cast<unsigned int>(value.intValue())); break;
        case FIELD_ueAddr: pp->setUeAddr(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_resourceType: pp->setResourceType(value.stringValue()); break;
        case FIELD_service: pp->setService(value.stringValue()); break;
        case FIELD_processGnbId: pp->setProcessGnbId(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_offloadGnbId: pp->setOffloadGnbId(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_offloadGnbAddr: pp->setOffloadGnbAddr(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_cmpUnits: pp->setCmpUnits(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_bands: pp->setBands(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_exeTime: pp->setExeTime(value.doubleValue()); break;
        case FIELD_maxOffloadTime: pp->setMaxOffloadTime(value.doubleValue()); break;
        case FIELD_deadline: pp->setDeadline(value.doubleValue()); break;
        case FIELD_outputSize: pp->setOutputSize(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_inputSize: pp->setInputSize(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_start: pp->setStart(value.boolValue()); break;
        case FIELD_stop: pp->setStop(value.boolValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Grant2Rsu'", field);
    }
}

const char *Grant2RsuDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr Grant2RsuDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    Grant2Rsu *pp = omnetpp::fromAnyPtr<Grant2Rsu>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void Grant2RsuDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    Grant2Rsu *pp = omnetpp::fromAnyPtr<Grant2Rsu>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Grant2Rsu'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

