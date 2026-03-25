//
// Generated file, do not edit! Created by opp_msgtool 6.0 from mecrt/packets/apps/DistToken.msg.
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
#include "DistToken_m.h"

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

Register_Class(DistToken)

DistToken::DistToken() : ::inet::FieldsChunk()
{
    this->setChunkLength(inet::B(19));

}

DistToken::DistToken(const DistToken& other) : ::inet::FieldsChunk(other)
{
    copy(other);
}

DistToken::~DistToken()
{
}

DistToken& DistToken::operator=(const DistToken& other)
{
    if (this == &other) return *this;
    ::inet::FieldsChunk::operator=(other);
    copy(other);
    return *this;
}

void DistToken::copy(const DistToken& other)
{
    this->appId = other.appId;
    this->utilReduction = other.utilReduction;
    this->targetCategory = other.targetCategory;
    this->preferenceValue = other.preferenceValue;
    this->isScheduled_ = other.isScheduled_;
}

void DistToken::parsimPack(omnetpp::cCommBuffer *b) const
{
    ::inet::FieldsChunk::parsimPack(b);
    doParsimPacking(b,this->appId);
    doParsimPacking(b,this->utilReduction);
    doParsimPacking(b,this->targetCategory);
    doParsimPacking(b,this->preferenceValue);
    doParsimPacking(b,this->isScheduled_);
}

void DistToken::parsimUnpack(omnetpp::cCommBuffer *b)
{
    ::inet::FieldsChunk::parsimUnpack(b);
    doParsimUnpacking(b,this->appId);
    doParsimUnpacking(b,this->utilReduction);
    doParsimUnpacking(b,this->targetCategory);
    doParsimUnpacking(b,this->preferenceValue);
    doParsimUnpacking(b,this->isScheduled_);
}

unsigned int DistToken::getAppId() const
{
    return this->appId;
}

void DistToken::setAppId(unsigned int appId)
{
    handleChange();
    this->appId = appId;
}

double DistToken::getUtilReduction() const
{
    return this->utilReduction;
}

void DistToken::setUtilReduction(double utilReduction)
{
    handleChange();
    this->utilReduction = utilReduction;
}

const char * DistToken::getTargetCategory() const
{
    return this->targetCategory.c_str();
}

void DistToken::setTargetCategory(const char * targetCategory)
{
    handleChange();
    this->targetCategory = targetCategory;
}

unsigned short DistToken::getPreferenceValue() const
{
    return this->preferenceValue;
}

void DistToken::setPreferenceValue(unsigned short preferenceValue)
{
    handleChange();
    this->preferenceValue = preferenceValue;
}

bool DistToken::isScheduled() const
{
    return this->isScheduled_;
}

void DistToken::setIsScheduled(bool isScheduled)
{
    handleChange();
    this->isScheduled_ = isScheduled;
}

class DistTokenDescriptor : public omnetpp::cClassDescriptor
{
  private:
    mutable const char **propertyNames;
    enum FieldConstants {
        FIELD_appId,
        FIELD_utilReduction,
        FIELD_targetCategory,
        FIELD_preferenceValue,
        FIELD_isScheduled,
    };
  public:
    DistTokenDescriptor();
    virtual ~DistTokenDescriptor();

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

Register_ClassDescriptor(DistTokenDescriptor)

DistTokenDescriptor::DistTokenDescriptor() : omnetpp::cClassDescriptor(omnetpp::opp_typename(typeid(DistToken)), "inet::FieldsChunk")
{
    propertyNames = nullptr;
}

DistTokenDescriptor::~DistTokenDescriptor()
{
    delete[] propertyNames;
}

bool DistTokenDescriptor::doesSupport(omnetpp::cObject *obj) const
{
    return dynamic_cast<DistToken *>(obj)!=nullptr;
}

const char **DistTokenDescriptor::getPropertyNames() const
{
    if (!propertyNames) {
        static const char *names[] = {  nullptr };
        omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
        const char **baseNames = base ? base->getPropertyNames() : nullptr;
        propertyNames = mergeLists(baseNames, names);
    }
    return propertyNames;
}

const char *DistTokenDescriptor::getProperty(const char *propertyName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? base->getProperty(propertyName) : nullptr;
}

int DistTokenDescriptor::getFieldCount() const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    return base ? 5+base->getFieldCount() : 5;
}

unsigned int DistTokenDescriptor::getFieldTypeFlags(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeFlags(field);
        field -= base->getFieldCount();
    }
    static unsigned int fieldTypeFlags[] = {
        FD_ISEDITABLE,    // FIELD_appId
        FD_ISEDITABLE,    // FIELD_utilReduction
        FD_ISEDITABLE,    // FIELD_targetCategory
        FD_ISEDITABLE,    // FIELD_preferenceValue
        FD_ISEDITABLE,    // FIELD_isScheduled
    };
    return (field >= 0 && field < 5) ? fieldTypeFlags[field] : 0;
}

const char *DistTokenDescriptor::getFieldName(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldName(field);
        field -= base->getFieldCount();
    }
    static const char *fieldNames[] = {
        "appId",
        "utilReduction",
        "targetCategory",
        "preferenceValue",
        "isScheduled",
    };
    return (field >= 0 && field < 5) ? fieldNames[field] : nullptr;
}

int DistTokenDescriptor::findField(const char *fieldName) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    int baseIndex = base ? base->getFieldCount() : 0;
    if (strcmp(fieldName, "appId") == 0) return baseIndex + 0;
    if (strcmp(fieldName, "utilReduction") == 0) return baseIndex + 1;
    if (strcmp(fieldName, "targetCategory") == 0) return baseIndex + 2;
    if (strcmp(fieldName, "preferenceValue") == 0) return baseIndex + 3;
    if (strcmp(fieldName, "isScheduled") == 0) return baseIndex + 4;
    return base ? base->findField(fieldName) : -1;
}

const char *DistTokenDescriptor::getFieldTypeString(int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldTypeString(field);
        field -= base->getFieldCount();
    }
    static const char *fieldTypeStrings[] = {
        "unsigned int",    // FIELD_appId
        "double",    // FIELD_utilReduction
        "string",    // FIELD_targetCategory
        "unsigned short",    // FIELD_preferenceValue
        "bool",    // FIELD_isScheduled
    };
    return (field >= 0 && field < 5) ? fieldTypeStrings[field] : nullptr;
}

const char **DistTokenDescriptor::getFieldPropertyNames(int field) const
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

const char *DistTokenDescriptor::getFieldProperty(int field, const char *propertyName) const
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

int DistTokenDescriptor::getFieldArraySize(omnetpp::any_ptr object, int field) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldArraySize(object, field);
        field -= base->getFieldCount();
    }
    DistToken *pp = omnetpp::fromAnyPtr<DistToken>(object); (void)pp;
    switch (field) {
        default: return 0;
    }
}

void DistTokenDescriptor::setFieldArraySize(omnetpp::any_ptr object, int field, int size) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldArraySize(object, field, size);
            return;
        }
        field -= base->getFieldCount();
    }
    DistToken *pp = omnetpp::fromAnyPtr<DistToken>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set array size of field %d of class 'DistToken'", field);
    }
}

const char *DistTokenDescriptor::getFieldDynamicTypeString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldDynamicTypeString(object,field,i);
        field -= base->getFieldCount();
    }
    DistToken *pp = omnetpp::fromAnyPtr<DistToken>(object); (void)pp;
    switch (field) {
        default: return nullptr;
    }
}

std::string DistTokenDescriptor::getFieldValueAsString(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValueAsString(object,field,i);
        field -= base->getFieldCount();
    }
    DistToken *pp = omnetpp::fromAnyPtr<DistToken>(object); (void)pp;
    switch (field) {
        case FIELD_appId: return ulong2string(pp->getAppId());
        case FIELD_utilReduction: return double2string(pp->getUtilReduction());
        case FIELD_targetCategory: return oppstring2string(pp->getTargetCategory());
        case FIELD_preferenceValue: return ulong2string(pp->getPreferenceValue());
        case FIELD_isScheduled: return bool2string(pp->isScheduled());
        default: return "";
    }
}

void DistTokenDescriptor::setFieldValueAsString(omnetpp::any_ptr object, int field, int i, const char *value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValueAsString(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    DistToken *pp = omnetpp::fromAnyPtr<DistToken>(object); (void)pp;
    switch (field) {
        case FIELD_appId: pp->setAppId(string2ulong(value)); break;
        case FIELD_utilReduction: pp->setUtilReduction(string2double(value)); break;
        case FIELD_targetCategory: pp->setTargetCategory((value)); break;
        case FIELD_preferenceValue: pp->setPreferenceValue(string2ulong(value)); break;
        case FIELD_isScheduled: pp->setIsScheduled(string2bool(value)); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'DistToken'", field);
    }
}

omnetpp::cValue DistTokenDescriptor::getFieldValue(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldValue(object,field,i);
        field -= base->getFieldCount();
    }
    DistToken *pp = omnetpp::fromAnyPtr<DistToken>(object); (void)pp;
    switch (field) {
        case FIELD_appId: return (omnetpp::intval_t)(pp->getAppId());
        case FIELD_utilReduction: return pp->getUtilReduction();
        case FIELD_targetCategory: return pp->getTargetCategory();
        case FIELD_preferenceValue: return (omnetpp::intval_t)(pp->getPreferenceValue());
        case FIELD_isScheduled: return pp->isScheduled();
        default: throw omnetpp::cRuntimeError("Cannot return field %d of class 'DistToken' as cValue -- field index out of range?", field);
    }
}

void DistTokenDescriptor::setFieldValue(omnetpp::any_ptr object, int field, int i, const omnetpp::cValue& value) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldValue(object, field, i, value);
            return;
        }
        field -= base->getFieldCount();
    }
    DistToken *pp = omnetpp::fromAnyPtr<DistToken>(object); (void)pp;
    switch (field) {
        case FIELD_appId: pp->setAppId(omnetpp::checked_int_cast<unsigned int>(value.intValue())); break;
        case FIELD_utilReduction: pp->setUtilReduction(value.doubleValue()); break;
        case FIELD_targetCategory: pp->setTargetCategory(value.stringValue()); break;
        case FIELD_preferenceValue: pp->setPreferenceValue(omnetpp::checked_int_cast<unsigned short>(value.intValue())); break;
        case FIELD_isScheduled: pp->setIsScheduled(value.boolValue()); break;
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'DistToken'", field);
    }
}

const char *DistTokenDescriptor::getFieldStructName(int field) const
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

omnetpp::any_ptr DistTokenDescriptor::getFieldStructValuePointer(omnetpp::any_ptr object, int field, int i) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount())
            return base->getFieldStructValuePointer(object, field, i);
        field -= base->getFieldCount();
    }
    DistToken *pp = omnetpp::fromAnyPtr<DistToken>(object); (void)pp;
    switch (field) {
        default: return omnetpp::any_ptr(nullptr);
    }
}

void DistTokenDescriptor::setFieldStructValuePointer(omnetpp::any_ptr object, int field, int i, omnetpp::any_ptr ptr) const
{
    omnetpp::cClassDescriptor *base = getBaseClassDescriptor();
    if (base) {
        if (field < base->getFieldCount()){
            base->setFieldStructValuePointer(object, field, i, ptr);
            return;
        }
        field -= base->getFieldCount();
    }
    DistToken *pp = omnetpp::fromAnyPtr<DistToken>(object); (void)pp;
    switch (field) {
        default: throw omnetpp::cRuntimeError("Cannot set field %d of class 'DistToken'", field);
    }
}

namespace omnetpp {

}  // namespace omnetpp

