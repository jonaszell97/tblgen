
#ifndef TABLEGEN_RECORD_H
#define TABLEGEN_RECORD_H

#include "tblgen/Lex/SourceLocation.h"
#include "tblgen/Support/Allocator.h"
#include "tblgen/Support/Optional.h"

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <vector>
#include <iostream>

namespace tblgen {

class TableGen;
class Type;
class Value;
class RecordKeeper;
class Class;
class Record;

class RecordField {
public:
   RecordField(std::string_view name,
               Type *type,
               Value *defaultValue,
               SourceLocation declLoc,
               size_t associatedTemplateParm = size_t(-1),
               bool append = false)
      : name(name), type(type), defaultValue(defaultValue),
        declLoc(declLoc), associatedTemplateParm(associatedTemplateParm),
        append(append)
   {}

   const std::string &getName() const
   {
      return name;
   }

   Type *getType() const
   {
      return type;
   }

   Value *getDefaultValue() const
   {
      return defaultValue;
   }

   SourceLocation getDeclLoc() const
   {
      return declLoc;
   }

   bool hasAssociatedTemplateParm() const
   {
      return associatedTemplateParm != -1;
   }

   bool isAppend() const { return append; }

   size_t getAssociatedTemplateParm() const
   {
      assert(hasAssociatedTemplateParm());
      return associatedTemplateParm;
   }

   friend class Class;
   friend class Record;

private:
   std::string name;
   Type *type;
   Value *defaultValue;
   SourceLocation declLoc;

   size_t associatedTemplateParm;
   bool append;
};

class Class {
public:
   class BaseClass {
   public:
      BaseClass(Class *Base, std::vector<Value*> &&templateParams)
         : Base(Base), templateArgs(move(templateParams))
      { }

      Class *getBase() const
      {
         return Base;
      }

      const std::vector<Value*> &getTemplateArgs() const
      {
         return templateArgs;
      }

   private:
      Class *Base;
      std::vector<Value*> templateArgs;
   };

   bool addField(std::string_view name,
                 Type *type,
                 Value *defaultValue,
                 SourceLocation declLoc,
                 size_t associatedTemplateParm = size_t(-1)) {
      if (getField(name))
         return false;

      fields.emplace_back(name, type, defaultValue, declLoc,
                          associatedTemplateParm);

      return true;
   }

   bool addOverride(std::string_view name,
                    Type *type,
                    Value *defaultValue,
                    SourceLocation declLoc,
                    bool append) {
      if (getOverride(name))
         return false;

      overrides.emplace_back(name, type, defaultValue, declLoc, -1, append);
      return true;
   }

   bool addTemplateParam(std::string_view name,
                         Type *type,
                         Value *defaultValue,
                         SourceLocation declLoc) {
      if (getTemplateParameter(name))
         return false;

      parameters.emplace_back(name, type, defaultValue, declLoc);

      return true;
   }

   void addBase(Class *Base, std::vector<Value*> &&templateParams)
   {
      bases.emplace_back(Base, move(templateParams));
   }

   const std::string &getName() const
   {
      return name;
   }

   SourceLocation getDeclLoc() const
   {
      return declLoc;
   }

   RecordField *getTemplateParameter(std::string_view name) const
   {
      for (auto &field : parameters)
         if (field.getName() == name)
            return const_cast<RecordField*>(&field);

      return nullptr;
   }

   RecordField* getField(std::string_view name) const
   {
      auto F = getOwnField(name);
      if (F)
         return F;

      for (auto &b : bases)
         if ((F = b.getBase()->getField(name)))
            return F;

      return nullptr;
   }

   RecordField* getOverride(std::string_view name) const
   {
      auto F = getOwnOverride(name);
      if (F)
         return F;

      for (auto &b : bases)
         if ((F = b.getBase()->getOverride(name)))
            return F;

      return nullptr;
   }

   RecordField* getOwnField(std::string_view name) const
   {
      for (auto &field : fields)
         if (field.getName() == name)
            return const_cast<RecordField*>(&field);

      return nullptr;
   }

   RecordField* getOwnOverride(std::string_view name) const
   {
      for (auto &field : overrides)
         if (field.getName() == name)
            return const_cast<RecordField*>(&field);

      return nullptr;
   }

   const std::vector<RecordField> &getParameters() const
   {
      return parameters;
   }

   const std::vector<RecordField> &getFields() const
   {
      return fields;
   }

   const std::vector<RecordField> &getOverrides() const
   {
      return fields;
   }

   const std::vector<BaseClass> &getBases() const
   {
      return bases;
   }

   bool inheritsFrom(Class *C)
   {
      for (auto &B : bases) {
         if (B.getBase() == C)
            return true;

         if (B.getBase()->inheritsFrom(C))
            return true;
      }

      return false;
   }

   RecordKeeper &getRecordKeeper() const { return RK; }

   void dump();
   void printTo(std::ostream &out);

   friend class RecordKeeper;

private:
   Class(RecordKeeper &RK, std::string_view name, SourceLocation declLoc)
      : RK(RK), name(name), declLoc(declLoc)
   {}

   RecordKeeper &RK;

   std::string name;
   SourceLocation declLoc;

   std::vector<BaseClass> bases;

   std::vector<RecordField> parameters;
   std::vector<RecordField> fields;
   std::vector<RecordField> overrides;
};

inline std::ostream &operator<<(std::ostream &str, Class &C)
{
   C.printTo(str);
   return str;
}

inline std::ostream &operator<<(std::ostream &str, Class *C)
{
   str << *C;
   return str;
}

class Record {
public:
   void addOwnField(SourceLocation loc, std::string_view key,
                    Type *Ty, Value *V) {
      auto &F = ownFields.emplace_back(key, Ty, V, loc);
      auto &Entry = *fieldValues.emplace(key, V).first;
      F.name = std::string_view(Entry.first);
   }

   void setFieldValue(const std::string &key, Value *V)
   {
      fieldValues[key] = V;
   }

   const std::string &getName() const
   {
      return name;
   }

   SourceLocation getDeclLoc() const
   {
      return declLoc;
   }

   const std::vector<Class::BaseClass> &getBases() const
   {
      return bases;
   }

   const std::unordered_map<std::string, Value *> &getFieldValues() const
   {
      return fieldValues;
   }

   void addBase(Class *Base, std::vector<Value*> &&templateParams)
   {
      bases.emplace_back(Base, move(templateParams));
   }

   bool hasField(const std::string &name) const
   {
      auto it = fieldValues.find(name);
      return it != fieldValues.end();
   }

   Type *getFieldType(std::string_view fieldName) const
   {
      for (auto &B : bases) {
         if (auto F = B.getBase()->getField(fieldName))
            return F->getType();
      }

      return nullptr;
   }

   Value *getFieldValue(const std::string &fieldName) const
   {
      auto it = fieldValues.find(fieldName);
      if (it != fieldValues.end())
         return it->second;

      return nullptr;
   }

   const std::vector<RecordField> &getOwnFields() const
   {
      return ownFields;
   }

   RecordField *getOwnField(std::string_view name)
   {
      for (auto &f : ownFields)
         if (f.getName() == name)
            return &f;

      return nullptr;
   }

   bool inheritsFrom(Class *C)
   {
      for (auto &B : bases) {
         if (B.getBase() == C)
            return true;

         if (B.getBase()->inheritsFrom(C))
            return true;
      }

      return false;
   }

   RecordKeeper &getRecordKeeper() const { return RK; }
   bool isAnonymous() const { return IsAnonymous; }

   void dump();
   void dumpAllValues();

   void printTo(std::ostream &out);

   friend class RecordKeeper;

private:
   Record(RecordKeeper &RK, const std::string &name, SourceLocation declLoc)
      : RK(RK), name(name), declLoc(declLoc)
   {
   }

   Record(RecordKeeper &RK, SourceLocation declLoc)
      : RK(RK), name(std::string("<anonymous record ") + std::to_string((uint64_t)this) + ">"), declLoc(declLoc), IsAnonymous(true)
   {
   }

   RecordKeeper &RK;

   std::string name;
   SourceLocation declLoc;

   std::vector<Class::BaseClass> bases;
   std::vector<RecordField> ownFields;

   std::unordered_map<std::string, Value*> fieldValues;

   bool IsAnonymous = false;
};

inline std::ostream &operator<<(std::ostream &str, Record &R)
{
   R.printTo(str);
   return str;
}

inline std::ostream &operator<<(std::ostream &str, Record *R)
{
   str << *R;
   return str;
}

struct EnumCase {
   std::string caseName;
   uint64_t caseValue;
};

class Enum {
public:
   void addCase(std::string_view caseName, support::Optional<uint64_t> caseVal = support::None);

   support::Optional<uint64_t> getCaseValue(const std::string &caseName) const;
   support::Optional<std::string_view> getCaseName(uint64_t caseVal) const;

   EnumCase *getCase(const std::string &caseName) const;
   EnumCase *getCase(uint64_t caseVal) const;

   bool hasCase(const std::string &caseName) const
   {
      return casesByName.find(caseName) != casesByName.end();
   }

   bool hasCase(uint64_t caseVal) const
   {
      return casesByValue.find(caseVal) != casesByValue.end();
   }

   std::string_view getName() const
   {
      return name;
   }

   SourceLocation getDeclLoc() const
   {
      return declLoc;
   }

   void printTo(std::ostream &out);

   friend class RecordKeeper;

private:
   Enum(RecordKeeper &RK, std::string_view name, SourceLocation declLoc)
      : RK(RK), name(name), declLoc(declLoc)
   {
   }

   RecordKeeper &RK;

   std::string name;
   SourceLocation declLoc;

   std::unordered_map<std::string, EnumCase*> casesByName;
   std::unordered_map<uint64_t, EnumCase*> casesByValue;
};

inline std::ostream &operator<<(std::ostream &str, Enum &R)
{
   R.printTo(str);
   return str;
}

inline std::ostream &operator<<(std::ostream &str, Enum *R)
{
   str << *R;
   return str;
}

class RecordKeeper {
public:
   RecordKeeper(TableGen &TG,
                const std::string &namespaceName = "",
                SourceLocation loc = {},
                RecordKeeper *Parent = nullptr)
      : TG(TG), namespaceName(namespaceName), declLoc(loc), Parent(Parent)
   {}

   Record *CreateRecord(const std::string &name, SourceLocation loc);
   Class *CreateClass(const std::string &name, SourceLocation loc);
   Enum *CreateEnum(const std::string &name, SourceLocation loc);

   [[nodiscard]]
   Record *CreateAnonymousRecord(SourceLocation loc);

   struct ValueDecl {
      ValueDecl(Value *Val, SourceLocation loc) : Val(Val), loc(loc)
      { }

      ValueDecl() : Val(nullptr) {}

      Value *getVal() const
      {
         return Val;
      }

      SourceLocation getLoc() const
      {
         return loc;
      }

   private:
      Value *Val;
      SourceLocation loc;
   };

   void addValue(const std::string &name,
                 Value *V,
                 SourceLocation loc);

   RecordKeeper *addNamespace(const std::string &name,
                              SourceLocation loc);

   Record *lookupRecord(const std::string &name) const
   {
      auto it = RecordsMap.find(name);
      if (it == RecordsMap.end()) {
         if (Parent)
            return Parent->lookupRecord(name);

         return nullptr;
      }

      return it->second;
   }

   Class *lookupClass(const std::string &name) const
   {
      auto it = Classes.find(name);
      if (it == Classes.end()) {
         if (Parent)
            return Parent->lookupClass(name);

         return nullptr;
      }

      return it->second;
   }

   Enum *lookupEnum(const std::string &name) const
   {
      auto it = Enums.find(name);
      if (it == Enums.end()) {
         return nullptr;
      }

      return it->second;
   }

   ValueDecl *lookupValueDecl(const std::string &name) const
   {
      auto it = Values.find(name);
      if (it == Values.end()) {
         if (Parent)
            return Parent->lookupValueDecl(name);

         return nullptr;
      }

      return const_cast<ValueDecl*>(&it->second);
   }

   RecordKeeper *lookupNamespace(const std::string & name) const
   {
      auto it = Namespaces.find(name);
      if (it == Namespaces.end()) {
         if (Parent)
            return Parent->lookupNamespace(name);

         return nullptr;
      }

      return const_cast<RecordKeeper*>(it->second);
   }

   SourceLocation lookupAnyDecl(const std::string & name) const
   {
      if (auto R = lookupRecord(name))
         return R->getDeclLoc();

      if (auto C = lookupClass(name))
         return C->getDeclLoc();

      if (auto E = lookupEnum(name))
         return E->getDeclLoc();

      if (auto V = lookupValueDecl(name))
         return V->getLoc();

      if (auto NS = lookupNamespace(name))
         return NS->getDeclLoc();

      return SourceLocation();
   }

   const std::unordered_map<std::string, Class *> &getAllClasses() const
   {
      return Classes;
   }

   const std::vector<Record*> &getAllRecords() const
   {
      return Records;
   }

   const std::unordered_map<std::string, ValueDecl> &getValueDecls() const
   {
      return Values;
   }

   const std::unordered_map<std::string, RecordKeeper*> &getAllNamespaces() const
   {
      return Namespaces;
   }

   std::string_view getNamespaceName() const
   {
      return namespaceName;
   }

   bool isGlobalRecordKeeper() const
   {
      return namespaceName.empty();
   }

   const SourceLocation &getDeclLoc() const
   {
      return declLoc;
   }

   void getAllDefinitionsOf(Class *C,
                            std::vector<Record*> &vec) const {
      for (auto &R : Records)
         if (R->inheritsFrom(C))
            vec.push_back(R);
   }

   void getAllDefinitionsOf(const std::string &className,
                            std::vector<Record*> &vec) const {
      return getAllDefinitionsOf(lookupClass(className), vec);
   }

   support::ArenaAllocator &getAllocator() const;

   void dump();
   void printTo(std::ostream &out);

private:
   TableGen &TG;

   std::string namespaceName;
   SourceLocation declLoc;
   RecordKeeper *Parent;

   std::unordered_map<std::string, Class*> Classes;
   std::vector<Record*> Records;
   std::unordered_map<std::string, Record*> RecordsMap;
   std::unordered_map<std::string, Enum*> Enums;
   std::unordered_map<std::string, ValueDecl> Values;
   std::unordered_map<std::string, RecordKeeper*> Namespaces;
};

inline std::ostream &operator<<(std::ostream &str, RecordKeeper &RK)
{
   RK.printTo(str);
   return str;
}

inline std::ostream &operator<<(std::ostream &str, RecordKeeper *RK)
{
   str << *RK;
   return str;
}

} // namespace tblgen

#endif //TABLEGEN_RECORD_H
