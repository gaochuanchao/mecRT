//
// Generated file, do not edit! Created by opp_msgtool 6.0 from mecrt/packets/apps/VecPacket.msg.
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
#include "VecPacket_m.h"

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

Register_Class(JobPacket)

JobPacket::JobPacket() : ::inet::FieldsChunk()
{
}

JobPacket::JobPacket(const JobPacket& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

JobPacket::~JobPacket()
{
}

JobPacket& JobPacket::operator=(const JobPacket& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void JobPacket::copy(const JobPacket& other)
{
    this->nframes = other.nframes;
    this->IDframe = other.IDframe;
    this->absDeadline = other.absDeadline;
    this->jobInitTimestamp = other.jobInitTimestamp;
    this->inputSize = other.inputSize;
    this->outputSize = other.outputSize;
    this->appId = other.appId;
}

void JobPacket::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->nframes);
    doParsimPacking(b,this->IDframe);
    doParsimPacking(b,this->absDeadline);
    doParsimPacking(b,this->jobInitTimestamp);
    doParsimPacking(b,this->inputSize);
    doParsimPacking(b,this->outputSize);
    doParsimPacking(b,this->appId);
}

void JobPacket::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->nframes);
    doParsimUnpacking(b,this->IDframe);
    doParsimUnpacking(b,this->absDeadline);
    doParsimUnpacking(b,this->jobInitTimestamp);
    doParsimUnpacking(b,this->inputSize);
    doParsimUnpacking(b,this->outputSize);
    doParsimUnpacking(b,this->appId);
}

int JobPacket::getNframes() const
{
    return this->nframes;
}

void JobPacket::setNframes(int nframes)
{
    handleChange();
    this->nframes = nframes;
}

int JobPacket::getIDframe() const
{
    return this->IDframe;
}

void JobPacket::setIDframe(int IDframe)
{
    handleChange();
    this->IDframe = IDframe;
}

omnetpp::simtime_t JobPacket::getAbsDeadline() const
{
    return this->absDeadline;
}

void JobPacket::setAbsDeadline(omnetpp::simtime_t absDeadline)
{
    handleChange();
    this->absDeadline = absDeadline;
}

omnetpp::simtime_t JobPacket::getJobInitTimestamp() const
{
    return this->jobInitTimestamp;
}

void JobPacket::setJobInitTimestamp(omnetpp::simtime_t jobInitTimestamp)
{
    handleChange();
    this->jobInitTimestamp = jobInitTimestamp;
}

int JobPacket::getInputSize() const
{
    return this->inputSize;
}

void JobPacket::setInputSize(int inputSize)
{
    handleChange();
    this->inputSize = inputSize;
}

int JobPacket::getOutputSize() const
{
    return this->outputSize;
}

void JobPacket::setOutputSize(int outputSize)
{
    handleChange();
    this->outputSize = outputSize;
}

unsigned int JobPacket::getAppId() const
{
    return this->appId;
}

void JobPacket::setAppId(unsigned int appId)
{
    handleChange();
    this->appId = appId;
}

class JobPacketDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_nframes,
        FIELD_IDframe,
        FIELD_absDeadline,
        FIELD_jobInitTimestamp,
        FIELD_inputSize,
        FIELD_outputSize,
        FIELD_appId,
    };
  public:
    JobPacketDescriptor();
    virtual ~JobPacketDescriptor();

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

Register_ClassDescriptor(JobPacketDescriptor)

JobPacketDescriptor::JobPacketDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(JobPacket)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

JobPacketDescriptor::~JobPacketDescriptor()
{
    delete[] propertyNames;
}

bool JobPacketDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<JobPacket *>(obj)!=nullptr;
}

const char **JobPacketDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *JobPacketDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int JobPacketDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 7+base->getFieldCount() : 7;
}

unsigned int JobPacketDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_nframes
        FD_ISEDITABLE,    // FIELD_IDframe
        FD_ISEDITABLE,    // FIELD_absDeadline
        FD_ISEDITABLE,    // FIELD_jobInitTimestamp
        FD_ISEDITABLE,    // FIELD_inputSize
        FD_ISEDITABLE,    // FIELD_outputSize
        FD_ISEDITABLE,    // FIELD_appId
    };
    return (field >= 0 && field < 7) ? fieldTypeFlags[field] : 0;
}

const char *JobPacketDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "nframes",
        "IDframe",
        "absDeadline",
        "jobInitTimestamp",
        "inputSize",
        "outputSize",
        "appId",
    };
    return (field >= 0 && field < 7) ? fieldNames[field] : nullptr;
}

int JobPacketDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "nframes") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "IDframe") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "absDeadline") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "jobInitTimestamp") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "inputSize") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "outputSize") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "appId") == 0) return baseIndex + 6;
    return base ? base->findField(fieldName) : -1;
}

const char *JobPacketDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_nframes
        "int",    // FIELD_IDframe
        "omnetpp::simtime_t",    // FIELD_absDeadline
        "omnetpp::simtime_t",    // FIELD_jobInitTimestamp
        "int",    // FIELD_inputSize
        "int",    // FIELD_outputSize
        "unsigned int",    // FIELD_appId
    };
    return (field >= 0 && field < 7) ? fieldTypeStrings[field] : nullptr;
}

const char **JobPacketDescriptor::getFieldPropertyNames(int field) const
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

const char *JobPacketDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int JobPacketDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    JobPacket *pp = omnetpp::fromAnyPtr<JobPacket>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void JobPacketDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    JobPacket *pp = omnetpp::fromAnyPtr<JobPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'JobPacket'", field);
    }
}

const char *JobPacketDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    JobPacket *pp = omnetpp::fromAnyPtr<JobPacket>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string JobPacketDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    JobPacket *pp = omnetpp::fromAnyPtr<JobPacket>(object); (void)pp;
    switch (field) {
        case FIELD_nframes: return long2string(pp->getNframes());
        case FIELD_IDframe: return long2string(pp->getIDframe());
        case FIELD_absDeadline: return simtime2string(pp->getAbsDeadline());
        case FIELD_jobInitTimestamp: return simtime2string(pp->getJobInitTimestamp());
        case FIELD_inputSize: return long2string(pp->getInputSize());
        case FIELD_outputSize: return long2string(pp->getOutputSize());
        case FIELD_appId: return ulong2string(pp->getAppId());
        default: return "";
    }
}

void JobPacketDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    JobPacket *pp = omnetpp::fromAnyPtr<JobPacket>(object); (void)pp;
    switch (field) {
        case FIELD_nframes: pp->setNframes(string2long(value)); break;
        case FIELD_IDframe: pp->setIDframe(string2long(value)); break;
        case FIELD_absDeadline: pp->setAbsDeadline(string2simtime(value)); break;
        case FIELD_jobInitTimestamp: pp->setJobInitTimestamp(string2simtime(value)); break;
        case FIELD_inputSize: pp->setInputSize(string2long(value)); break;
        case FIELD_outputSize: pp->setOutputSize(string2long(value)); break;
        case FIELD_appId: pp->setAppId(string2ulong(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'JobPacket'", field);
    }
}

omnetpp::cValue JobPacketDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    JobPacket *pp = omnetpp::fromAnyPtr<JobPacket>(object); (void)pp;
    switch (field) {
        case FIELD_nframes: return pp->getNframes();
        case FIELD_IDframe: return pp->getIDframe();
        case FIELD_absDeadline: return pp->getAbsDeadline().dbl();
        case FIELD_jobInitTimestamp: return pp->getJobInitTimestamp().dbl();
        case FIELD_inputSize: return pp->getInputSize();
        case FIELD_outputSize: return pp->getOutputSize();
        case FIELD_appId: return (omnetpp::intval_t)(pp->getAppId());
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'JobPacket' as cValue -- field index out of range?", field);
    }
}

void JobPacketDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    JobPacket *pp = omnetpp::fromAnyPtr<JobPacket>(object); (void)pp;
    switch (field) {
        case FIELD_nframes: pp->setNframes(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_IDframe: pp->setIDframe(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_absDeadline: pp->setAbsDeadline(value.doubleValue()); break;
        case FIELD_jobInitTimestamp: pp->setJobInitTimestamp(value.doubleValue()); break;
        case FIELD_inputSize: pp->setInputSize(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_outputSize: pp->setOutputSize(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_appId: pp->setAppId(omnetpp::checked_int_cast<unsigned int>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'JobPacket'", field);
    }
}

const char *JobPacketDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr JobPacketDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    JobPacket *pp = omnetpp::fromAnyPtr<JobPacket>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void JobPacketDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    JobPacket *pp = omnetpp::fromAnyPtr<JobPacket>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'JobPacket'", field);
    }
}

Register_Class(VecRequest)

VecRequest::VecRequest() : ::inet::FieldsChunk()
{
    this->setChunkLength(inet::B(44));

}

VecRequest::VecRequest(const VecRequest& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

VecRequest::~VecRequest()
{
}

VecRequest& VecRequest::operator=(const VecRequest& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void VecRequest::copy(const VecRequest& other)
{
    this->inputSize = other.inputSize;
    this->outputSize = other.outputSize;
    this->ueIpAddress = other.ueIpAddress;
    this->period = other.period;
    this->resourceType = other.resourceType;
    this->service = other.service;
    this->appId = other.appId;
    this->stopTime = other.stopTime;
    this->energy = other.energy;
    this->offloadPower = other.offloadPower;
}

void VecRequest::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->inputSize);
    doParsimPacking(b,this->outputSize);
    doParsimPacking(b,this->ueIpAddress);
    doParsimPacking(b,this->period);
    doParsimPacking(b,this->resourceType);
    doParsimPacking(b,this->service);
    doParsimPacking(b,this->appId);
    doParsimPacking(b,this->stopTime);
    doParsimPacking(b,this->energy);
    doParsimPacking(b,this->offloadPower);
}

void VecRequest::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->inputSize);
    doParsimUnpacking(b,this->outputSize);
    doParsimUnpacking(b,this->ueIpAddress);
    doParsimUnpacking(b,this->period);
    doParsimUnpacking(b,this->resourceType);
    doParsimUnpacking(b,this->service);
    doParsimUnpacking(b,this->appId);
    doParsimUnpacking(b,this->stopTime);
    doParsimUnpacking(b,this->energy);
    doParsimUnpacking(b,this->offloadPower);
}

int VecRequest::getInputSize() const
{
    return this->inputSize;
}

void VecRequest::setInputSize(int inputSize)
{
    handleChange();
    this->inputSize = inputSize;
}

int VecRequest::getOutputSize() const
{
    return this->outputSize;
}

void VecRequest::setOutputSize(int outputSize)
{
    handleChange();
    this->outputSize = outputSize;
}

uint32_t VecRequest::getUeIpAddress() const
{
    return this->ueIpAddress;
}

void VecRequest::setUeIpAddress(uint32_t ueIpAddress)
{
    handleChange();
    this->ueIpAddress = ueIpAddress;
}

omnetpp::simtime_t VecRequest::getPeriod() const
{
    return this->period;
}

void VecRequest::setPeriod(omnetpp::simtime_t period)
{
    handleChange();
    this->period = period;
}

unsigned short VecRequest::getResourceType() const
{
    return this->resourceType;
}

void VecRequest::setResourceType(unsigned short resourceType)
{
    handleChange();
    this->resourceType = resourceType;
}

unsigned short VecRequest::getService() const
{
    return this->service;
}

void VecRequest::setService(unsigned short service)
{
    handleChange();
    this->service = service;
}

unsigned int VecRequest::getAppId() const
{
    return this->appId;
}

void VecRequest::setAppId(unsigned int appId)
{
    handleChange();
    this->appId = appId;
}

omnetpp::simtime_t VecRequest::getStopTime() const
{
    return this->stopTime;
}

void VecRequest::setStopTime(omnetpp::simtime_t stopTime)
{
    handleChange();
    this->stopTime = stopTime;
}

double VecRequest::getEnergy() const
{
    return this->energy;
}

void VecRequest::setEnergy(double energy)
{
    handleChange();
    this->energy = energy;
}

double VecRequest::getOffloadPower() const
{
    return this->offloadPower;
}

void VecRequest::setOffloadPower(double offloadPower)
{
    handleChange();
    this->offloadPower = offloadPower;
}

class VecRequestDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_inputSize,
        FIELD_outputSize,
        FIELD_ueIpAddress,
        FIELD_period,
        FIELD_resourceType,
        FIELD_service,
        FIELD_appId,
        FIELD_stopTime,
        FIELD_energy,
        FIELD_offloadPower,
    };
  public:
    VecRequestDescriptor();
    virtual ~VecRequestDescriptor();

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

Register_ClassDescriptor(VecRequestDescriptor)

VecRequestDescriptor::VecRequestDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(VecRequest)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

VecRequestDescriptor::~VecRequestDescriptor()
{
    delete[] propertyNames;
}

bool VecRequestDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<VecRequest *>(obj)!=nullptr;
}

const char **VecRequestDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *VecRequestDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int VecRequestDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 10+base->getFieldCount() : 10;
}

unsigned int VecRequestDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_inputSize
        FD_ISEDITABLE,    // FIELD_outputSize
        FD_ISEDITABLE,    // FIELD_ueIpAddress
        FD_ISEDITABLE,    // FIELD_period
        FD_ISEDITABLE,    // FIELD_resourceType
        FD_ISEDITABLE,    // FIELD_service
        FD_ISEDITABLE,    // FIELD_appId
        FD_ISEDITABLE,    // FIELD_stopTime
        FD_ISEDITABLE,    // FIELD_energy
        FD_ISEDITABLE,    // FIELD_offloadPower
    };
    return (field >= 0 && field < 10) ? fieldTypeFlags[field] : 0;
}

const char *VecRequestDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "inputSize",
        "outputSize",
        "ueIpAddress",
        "period",
        "resourceType",
        "service",
        "appId",
        "stopTime",
        "energy",
        "offloadPower",
    };
    return (field >= 0 && field < 10) ? fieldNames[field] : nullptr;
}

int VecRequestDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "inputSize") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "outputSize") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "ueIpAddress") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "period") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "resourceType") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "service") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "appId") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "stopTime") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "energy") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "offloadPower") == 0) return baseIndex + 9;
    return base ? base->findField(fieldName) : -1;
}

const char *VecRequestDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "int",    // FIELD_inputSize
        "int",    // FIELD_outputSize
        "uint32_t",    // FIELD_ueIpAddress
        "omnetpp::simtime_t",    // FIELD_period
        "unsigned short",    // FIELD_resourceType
        "unsigned short",    // FIELD_service
        "unsigned int",    // FIELD_appId
        "omnetpp::simtime_t",    // FIELD_stopTime
        "double",    // FIELD_energy
        "double",    // FIELD_offloadPower
    };
    return (field >= 0 && field < 10) ? fieldTypeStrings[field] : nullptr;
}

const char **VecRequestDescriptor::getFieldPropertyNames(int field) const
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

const char *VecRequestDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int VecRequestDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    VecRequest *pp = omnetpp::fromAnyPtr<VecRequest>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void VecRequestDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    VecRequest *pp = omnetpp::fromAnyPtr<VecRequest>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'VecRequest'", field);
    }
}

const char *VecRequestDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    VecRequest *pp = omnetpp::fromAnyPtr<VecRequest>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string VecRequestDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    VecRequest *pp = omnetpp::fromAnyPtr<VecRequest>(object); (void)pp;
    switch (field) {
        case FIELD_inputSize: return long2string(pp->getInputSize());
        case FIELD_outputSize: return long2string(pp->getOutputSize());
        case FIELD_ueIpAddress: return ulong2string(pp->getUeIpAddress());
        case FIELD_period: return simtime2string(pp->getPeriod());
        case FIELD_resourceType: return ulong2string(pp->getResourceType());
        case FIELD_service: return ulong2string(pp->getService());
        case FIELD_appId: return ulong2string(pp->getAppId());
        case FIELD_stopTime: return simtime2string(pp->getStopTime());
        case FIELD_energy: return double2string(pp->getEnergy());
        case FIELD_offloadPower: return double2string(pp->getOffloadPower());
        default: return "";
    }
}

void VecRequestDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    VecRequest *pp = omnetpp::fromAnyPtr<VecRequest>(object); (void)pp;
    switch (field) {
        case FIELD_inputSize: pp->setInputSize(string2long(value)); break;
        case FIELD_outputSize: pp->setOutputSize(string2long(value)); break;
        case FIELD_ueIpAddress: pp->setUeIpAddress(string2ulong(value)); break;
        case FIELD_period: pp->setPeriod(string2simtime(value)); break;
        case FIELD_resourceType: pp->setResourceType(string2ulong(value)); break;
        case FIELD_service: pp->setService(string2ulong(value)); break;
        case FIELD_appId: pp->setAppId(string2ulong(value)); break;
        case FIELD_stopTime: pp->setStopTime(string2simtime(value)); break;
        case FIELD_energy: pp->setEnergy(string2double(value)); break;
        case FIELD_offloadPower: pp->setOffloadPower(string2double(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'VecRequest'", field);
    }
}

omnetpp::cValue VecRequestDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    VecRequest *pp = omnetpp::fromAnyPtr<VecRequest>(object); (void)pp;
    switch (field) {
        case FIELD_inputSize: return pp->getInputSize();
        case FIELD_outputSize: return pp->getOutputSize();
        case FIELD_ueIpAddress: return (omnetpp::intval_t)(pp->getUeIpAddress());
        case FIELD_period: return pp->getPeriod().dbl();
        case FIELD_resourceType: return (omnetpp::intval_t)(pp->getResourceType());
        case FIELD_service: return (omnetpp::intval_t)(pp->getService());
        case FIELD_appId: return (omnetpp::intval_t)(pp->getAppId());
        case FIELD_stopTime: return pp->getStopTime().dbl();
        case FIELD_energy: return pp->getEnergy();
        case FIELD_offloadPower: return pp->getOffloadPower();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'VecRequest' as cValue -- field index out of range?", field);
    }
}

void VecRequestDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    VecRequest *pp = omnetpp::fromAnyPtr<VecRequest>(object); (void)pp;
    switch (field) {
        case FIELD_inputSize: pp->setInputSize(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_outputSize: pp->setOutputSize(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_ueIpAddress: pp->setUeIpAddress(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_period: pp->setPeriod(value.doubleValue()); break;
        case FIELD_resourceType: pp->setResourceType(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_service: pp->setService(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_appId: pp->setAppId(omnetpp::checked_int_cast<unsigned int>(value.intValue())); break;
        case FIELD_stopTime: pp->setStopTime(value.doubleValue()); break;
        case FIELD_energy: pp->setEnergy(value.doubleValue()); break;
        case FIELD_offloadPower: pp->setOffloadPower(value.doubleValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'VecRequest'", field);
    }
}

const char *VecRequestDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr VecRequestDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    VecRequest *pp = omnetpp::fromAnyPtr<VecRequest>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void VecRequestDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    VecRequest *pp = omnetpp::fromAnyPtr<VecRequest>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'VecRequest'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

