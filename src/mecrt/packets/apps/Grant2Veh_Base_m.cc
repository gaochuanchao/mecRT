//
// Generated file, do not edit! Created by opp_msgtool 6.0 from mecrt/packets/apps/Grant2Veh_Base.msg.
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
#include "Grant2Veh_Base_m.h"

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

Register_Class(Grant2Veh_Base)

Grant2Veh_Base::Grant2Veh_Base() : ::inet::FieldsChunk()
{
    this->setChunkLength(inet::B(40));

}

Grant2Veh_Base::Grant2Veh_Base(const Grant2Veh_Base& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

Grant2Veh_Base::~Grant2Veh_Base()
{
}

Grant2Veh_Base& Grant2Veh_Base::operator=(const Grant2Veh_Base& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void Grant2Veh_Base::copy(const Grant2Veh_Base& other)
{
    this->appId = other.appId;
    this->ueAddr = other.ueAddr;
    this->maxOffloadTime = other.maxOffloadTime;
    this->bands = other.bands;
    this->processGnbId = other.processGnbId;
    this->offloadGnbId = other.offloadGnbId;
    this->processGnbAddr = other.processGnbAddr;
    this->processGnbPort = other.processGnbPort;
    this->inputSize = other.inputSize;
    this->outputSize = other.outputSize;
    this->bytePerTTI = other.bytePerTTI;
}

void Grant2Veh_Base::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->appId);
    doParsimPacking(b,this->ueAddr);
    doParsimPacking(b,this->maxOffloadTime);
    doParsimPacking(b,this->bands);
    doParsimPacking(b,this->processGnbId);
    doParsimPacking(b,this->offloadGnbId);
    doParsimPacking(b,this->processGnbAddr);
    doParsimPacking(b,this->processGnbPort);
    doParsimPacking(b,this->inputSize);
    doParsimPacking(b,this->outputSize);
    doParsimPacking(b,this->bytePerTTI);
}

void Grant2Veh_Base::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->appId);
    doParsimUnpacking(b,this->ueAddr);
    doParsimUnpacking(b,this->maxOffloadTime);
    doParsimUnpacking(b,this->bands);
    doParsimUnpacking(b,this->processGnbId);
    doParsimUnpacking(b,this->offloadGnbId);
    doParsimUnpacking(b,this->processGnbAddr);
    doParsimUnpacking(b,this->processGnbPort);
    doParsimUnpacking(b,this->inputSize);
    doParsimUnpacking(b,this->outputSize);
    doParsimUnpacking(b,this->bytePerTTI);
}

unsigned int Grant2Veh_Base::getAppId() const
{
    return this->appId;
}

void Grant2Veh_Base::setAppId(unsigned int appId)
{
    handleChange();
    this->appId = appId;
}

uint32_t Grant2Veh_Base::getUeAddr() const
{
    return this->ueAddr;
}

void Grant2Veh_Base::setUeAddr(uint32_t ueAddr)
{
    handleChange();
    this->ueAddr = ueAddr;
}

omnetpp::simtime_t Grant2Veh_Base::getMaxOffloadTime() const
{
    return this->maxOffloadTime;
}

void Grant2Veh_Base::setMaxOffloadTime(omnetpp::simtime_t maxOffloadTime)
{
    handleChange();
    this->maxOffloadTime = maxOffloadTime;
}

int Grant2Veh_Base::getBands() const
{
    return this->bands;
}

void Grant2Veh_Base::setBands(int bands)
{
    handleChange();
    this->bands = bands;
}

unsigned short Grant2Veh_Base::getProcessGnbId() const
{
    return this->processGnbId;
}

void Grant2Veh_Base::setProcessGnbId(unsigned short processGnbId)
{
    handleChange();
    this->processGnbId = processGnbId;
}

unsigned short Grant2Veh_Base::getOffloadGnbId() const
{
    return this->offloadGnbId;
}

void Grant2Veh_Base::setOffloadGnbId(unsigned short offloadGnbId)
{
    handleChange();
    this->offloadGnbId = offloadGnbId;
}

uint32_t Grant2Veh_Base::getProcessGnbAddr() const
{
    return this->processGnbAddr;
}

void Grant2Veh_Base::setProcessGnbAddr(uint32_t processGnbAddr)
{
    handleChange();
    this->processGnbAddr = processGnbAddr;
}

int Grant2Veh_Base::getProcessGnbPort() const
{
    return this->processGnbPort;
}

void Grant2Veh_Base::setProcessGnbPort(int processGnbPort)
{
    handleChange();
    this->processGnbPort = processGnbPort;
}

int Grant2Veh_Base::getInputSize() const
{
    return this->inputSize;
}

void Grant2Veh_Base::setInputSize(int inputSize)
{
    handleChange();
    this->inputSize = inputSize;
}

int Grant2Veh_Base::getOutputSize() const
{
    return this->outputSize;
}

void Grant2Veh_Base::setOutputSize(int outputSize)
{
    handleChange();
    this->outputSize = outputSize;
}

int Grant2Veh_Base::getBytePerTTI() const
{
    return this->bytePerTTI;
}

void Grant2Veh_Base::setBytePerTTI(int bytePerTTI)
{
    handleChange();
    this->bytePerTTI = bytePerTTI;
}

class Grant2Veh_BaseDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_appId,
        FIELD_ueAddr,
        FIELD_maxOffloadTime,
        FIELD_bands,
        FIELD_processGnbId,
        FIELD_offloadGnbId,
        FIELD_processGnbAddr,
        FIELD_processGnbPort,
        FIELD_inputSize,
        FIELD_outputSize,
        FIELD_bytePerTTI,
    };
  public:
    Grant2Veh_BaseDescriptor();
    virtual ~Grant2Veh_BaseDescriptor();

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

Register_ClassDescriptor(Grant2Veh_BaseDescriptor)

Grant2Veh_BaseDescriptor::Grant2Veh_BaseDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(Grant2Veh_Base)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

Grant2Veh_BaseDescriptor::~Grant2Veh_BaseDescriptor()
{
    delete[] propertyNames;
}

bool Grant2Veh_BaseDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<Grant2Veh_Base *>(obj)!=nullptr;
}

const char **Grant2Veh_BaseDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *Grant2Veh_BaseDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int Grant2Veh_BaseDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 11+base->getFieldCount() : 11;
}

unsigned int Grant2Veh_BaseDescriptor::getFieldTypeFlags(int field) const
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
        FD_ISEDITABLE,    // FIELD_maxOffloadTime
        FD_ISEDITABLE,    // FIELD_bands
        FD_ISEDITABLE,    // FIELD_processGnbId
        FD_ISEDITABLE,    // FIELD_offloadGnbId
        FD_ISEDITABLE,    // FIELD_processGnbAddr
        FD_ISEDITABLE,    // FIELD_processGnbPort
        FD_ISEDITABLE,    // FIELD_inputSize
        FD_ISEDITABLE,    // FIELD_outputSize
        FD_ISEDITABLE,    // FIELD_bytePerTTI
    };
    return (field >= 0 && field < 11) ? fieldTypeFlags[field] : 0;
}

const char *Grant2Veh_BaseDescriptor::getFieldName(int field) const
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
        "maxOffloadTime",
        "bands",
        "processGnbId",
        "offloadGnbId",
        "processGnbAddr",
        "processGnbPort",
        "inputSize",
        "outputSize",
        "bytePerTTI",
    };
    return (field >= 0 && field < 11) ? fieldNames[field] : nullptr;
}

int Grant2Veh_BaseDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "appId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "ueAddr") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "maxOffloadTime") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "bands") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "processGnbId") == 0) return baseIndex + 4;
    if (strcmp(fieldName, "offloadGnbId") == 0) return baseIndex + 5;
    if (strcmp(fieldName, "processGnbAddr") == 0) return baseIndex + 6;
    if (strcmp(fieldName, "processGnbPort") == 0) return baseIndex + 7;
    if (strcmp(fieldName, "inputSize") == 0) return baseIndex + 8;
    if (strcmp(fieldName, "outputSize") == 0) return baseIndex + 9;
    if (strcmp(fieldName, "bytePerTTI") == 0) return baseIndex + 10;
    return base ? base->findField(fieldName) : -1;
}

const char *Grant2Veh_BaseDescriptor::getFieldTypeString(int field) const
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
        "omnetpp::simtime_t",    // FIELD_maxOffloadTime
        "int",    // FIELD_bands
        "unsigned short",    // FIELD_processGnbId
        "unsigned short",    // FIELD_offloadGnbId
        "uint32",    // FIELD_processGnbAddr
        "int",    // FIELD_processGnbPort
        "int",    // FIELD_inputSize
        "int",    // FIELD_outputSize
        "int",    // FIELD_bytePerTTI
    };
    return (field >= 0 && field < 11) ? fieldTypeStrings[field] : nullptr;
}

const char **Grant2Veh_BaseDescriptor::getFieldPropertyNames(int field) const
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

const char *Grant2Veh_BaseDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int Grant2Veh_BaseDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    Grant2Veh_Base *pp = omnetpp::fromAnyPtr<Grant2Veh_Base>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void Grant2Veh_BaseDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    Grant2Veh_Base *pp = omnetpp::fromAnyPtr<Grant2Veh_Base>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'Grant2Veh_Base'", field);
    }
}

const char *Grant2Veh_BaseDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    Grant2Veh_Base *pp = omnetpp::fromAnyPtr<Grant2Veh_Base>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string Grant2Veh_BaseDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    Grant2Veh_Base *pp = omnetpp::fromAnyPtr<Grant2Veh_Base>(object); (void)pp;
    switch (field) {
        case FIELD_appId: return ulong2string(pp->getAppId());
        case FIELD_ueAddr: return ulong2string(pp->getUeAddr());
        case FIELD_maxOffloadTime: return simtime2string(pp->getMaxOffloadTime());
        case FIELD_bands: return long2string(pp->getBands());
        case FIELD_processGnbId: return ulong2string(pp->getProcessGnbId());
        case FIELD_offloadGnbId: return ulong2string(pp->getOffloadGnbId());
        case FIELD_processGnbAddr: return ulong2string(pp->getProcessGnbAddr());
        case FIELD_processGnbPort: return long2string(pp->getProcessGnbPort());
        case FIELD_inputSize: return long2string(pp->getInputSize());
        case FIELD_outputSize: return long2string(pp->getOutputSize());
        case FIELD_bytePerTTI: return long2string(pp->getBytePerTTI());
        default: return "";
    }
}

void Grant2Veh_BaseDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Grant2Veh_Base *pp = omnetpp::fromAnyPtr<Grant2Veh_Base>(object); (void)pp;
    switch (field) {
        case FIELD_appId: pp->setAppId(string2ulong(value)); break;
        case FIELD_ueAddr: pp->setUeAddr(string2ulong(value)); break;
        case FIELD_maxOffloadTime: pp->setMaxOffloadTime(string2simtime(value)); break;
        case FIELD_bands: pp->setBands(string2long(value)); break;
        case FIELD_processGnbId: pp->setProcessGnbId(string2ulong(value)); break;
        case FIELD_offloadGnbId: pp->setOffloadGnbId(string2ulong(value)); break;
        case FIELD_processGnbAddr: pp->setProcessGnbAddr(string2ulong(value)); break;
        case FIELD_processGnbPort: pp->setProcessGnbPort(string2long(value)); break;
        case FIELD_inputSize: pp->setInputSize(string2long(value)); break;
        case FIELD_outputSize: pp->setOutputSize(string2long(value)); break;
        case FIELD_bytePerTTI: pp->setBytePerTTI(string2long(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Grant2Veh_Base'", field);
    }
}

omnetpp::cValue Grant2Veh_BaseDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    Grant2Veh_Base *pp = omnetpp::fromAnyPtr<Grant2Veh_Base>(object); (void)pp;
    switch (field) {
        case FIELD_appId: return (omnetpp::intval_t)(pp->getAppId());
        case FIELD_ueAddr: return (omnetpp::intval_t)(pp->getUeAddr());
        case FIELD_maxOffloadTime: return pp->getMaxOffloadTime().dbl();
        case FIELD_bands: return pp->getBands();
        case FIELD_processGnbId: return (omnetpp::intval_t)(pp->getProcessGnbId());
        case FIELD_offloadGnbId: return (omnetpp::intval_t)(pp->getOffloadGnbId());
        case FIELD_processGnbAddr: return (omnetpp::intval_t)(pp->getProcessGnbAddr());
        case FIELD_processGnbPort: return pp->getProcessGnbPort();
        case FIELD_inputSize: return pp->getInputSize();
        case FIELD_outputSize: return pp->getOutputSize();
        case FIELD_bytePerTTI: return pp->getBytePerTTI();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'Grant2Veh_Base' as cValue -- field index out of range?", field);
    }
}

void Grant2Veh_BaseDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    Grant2Veh_Base *pp = omnetpp::fromAnyPtr<Grant2Veh_Base>(object); (void)pp;
    switch (field) {
        case FIELD_appId: pp->setAppId(omnetpp::checked_int_cast<unsigned int>(value.intValue())); break;
        case FIELD_ueAddr: pp->setUeAddr(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_maxOffloadTime: pp->setMaxOffloadTime(value.doubleValue()); break;
        case FIELD_bands: pp->setBands(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_processGnbId: pp->setProcessGnbId(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_offloadGnbId: pp->setOffloadGnbId(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_processGnbAddr: pp->setProcessGnbAddr(omnetpp::checked_int_cast<uint32_t>(value.intValue())); break;
        case FIELD_processGnbPort: pp->setProcessGnbPort(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_inputSize: pp->setInputSize(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_outputSize: pp->setOutputSize(omnetpp::checked_int_cast<int>(value.intValue())); break;
        case FIELD_bytePerTTI: pp->setBytePerTTI(omnetpp::checked_int_cast<int>(value.intValue())); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Grant2Veh_Base'", field);
    }
}

const char *Grant2Veh_BaseDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr Grant2Veh_BaseDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    Grant2Veh_Base *pp = omnetpp::fromAnyPtr<Grant2Veh_Base>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void Grant2Veh_BaseDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    Grant2Veh_Base *pp = omnetpp::fromAnyPtr<Grant2Veh_Base>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'Grant2Veh_Base'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

