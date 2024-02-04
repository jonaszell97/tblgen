
#ifndef TBLGEN_VALUE_H
#define TBLGEN_VALUE_H

#include "tblgen/Support/Casting.h"

#include <string>
#include <unordered_map>
#include <vector>

namespace tblgen {

class TableGen;
class Type;
class Record;
class Enum;
struct EnumCase;

class Value {
public:
   enum TypeID {
      IntegerLiteralID,
      FPLiteralID,
      StringLiteralID,
      CodeBlockID,
      ListLiteralID,
      DictLiteralID,
      IdentifierValID,
      RecordValID,
      EnumValID,
      UndefValID,

      DictAccessExprID,
   };

   TypeID getTypeID() const
   {
      return typeID;
   }

   Type *getType() const
   {
      return type;
   }

protected:
   explicit Value(TypeID typeID, Type *Ty) : typeID(typeID), type(Ty)
   { }

#  ifndef NDEBUG
   virtual
#  endif
   ~Value();

protected:
   TypeID typeID;
   Type *type;
};

class IntegerLiteral: public Value {
public:
   explicit IntegerLiteral(Type *Ty, uint64_t Val)
      : Value(IntegerLiteralID, Ty), Val(Val)
   { }

   explicit IntegerLiteral(Type *Ty, int64_t Val)
      : Value(IntegerLiteralID, Ty), Val(Val)
   { }

   uint64_t getVal() const
   {
      return Val;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == IntegerLiteralID;}

private:
   uint64_t Val;
};

class FPLiteral: public Value {
public:
   explicit FPLiteral(Type *Ty, double Val)
      : Value(FPLiteralID, Ty), Val(Val)
   { }

   double getVal() const
   {
      return Val;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == FPLiteralID;}

private:
   double Val;
};

class StringLiteral: public Value {
public:
   explicit StringLiteral(Type *Ty, std::string &&Val)
      : Value(StringLiteralID, Ty), Val(Val)
   { }

   const std::string &getVal() const
   {
      return Val;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == StringLiteralID;}

private:
   std::string Val;
};

class CodeBlock: public Value {
public:
   explicit CodeBlock(Type *Ty, std::string &&Val)
      : Value(CodeBlockID, Ty), Code(move(Val))
   { }

   std::string_view getCode() const
   {
      return Code;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == CodeBlockID;}

private:
   std::string Code;
};

class ListLiteral: public Value {
public:
   explicit ListLiteral(Type *Ty, std::vector<Value*> &&Values)
      : Value(ListLiteralID, Ty), Values(Values)
   { }

   const std::vector<Value *> &getValues() const
   {
      return Values;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == ListLiteralID;}

private:
   std::vector<Value*> Values;
};

class DictLiteral: public Value {
public:
   explicit DictLiteral(Type *Ty, std::vector<Value*> &&Keys,
                        std::vector<Value*> &&Values)
      : Value(DictLiteralID, Ty)
   {
      assert(Keys.size() == Values.size());
      for (size_t i = 0; i < Keys.size(); ++i)
         this->Values.emplace(support::cast<StringLiteral>(Keys[i])->getVal(),
            Values[i]);
   }

   explicit DictLiteral(Type *Ty, std::unordered_map<std::string, Value*> &&Entries)
      : Value(DictLiteralID, Ty), Values(move(Entries))
   {

   }

   const std::unordered_map<std::string, Value*> &getValues() const
   {
      return Values;
   }

   Value *getValue(const std::string &key) const
   {
      auto it = Values.find(key);
      if (it != Values.end())
         return it->second;

      return nullptr;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == DictLiteralID;}

private:
   std::unordered_map<std::string, Value*> Values;
};

class IdentifierVal: public Value {
public:
   explicit IdentifierVal(Type *Ty, std::string_view Val)
      : Value(IdentifierValID, Ty), Val(Val)
   { }

   std::string_view getVal() const
   {
      return Val;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == IdentifierValID;}

private:
   std::string Val;
};

class UndefValue: public Value {
public:
   explicit UndefValue(Type *Ty)
      : Value(UndefValID, Ty)
   { }

   static bool classof(Value const* V)
   { return V->getTypeID() == UndefValID;}
};

class RecordVal: public Value {
public:
   explicit RecordVal(Type *Ty, Record *R)
      : Value(RecordValID, Ty), R(R)
   { }

   Record *getRecord() const
   {
      return R;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == RecordValID;}

private:
   Record *R;
};

class EnumVal: public Value {
public:
   explicit EnumVal(Type *Ty, Enum *E, EnumCase *C)
      : Value(EnumValID, Ty), E(E), C(C)
   { }

   Enum *getEnum() const { return E; }
   EnumCase *getCase() const { return C; }

   static bool classof(Value const* V) { return V->getTypeID() == EnumValID;}

private:
   Enum *E;
   EnumCase *C;
};

class DictAccessExpr: public Value {
public:
   explicit DictAccessExpr(Value *dict, std::string_view key)
      : Value(DictAccessExprID, nullptr), dict(dict), key(key)
   {}

   Value *getDict() const
   {
      return dict;
   }

   const std::string_view &getKey() const
   {
      return key;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == DictAccessExprID;}

private:
   Value *dict;
   std::string_view key;
};

std::ostream &operator<<(std::ostream &str, Value const *V);

} // namespace tblgen

#endif //TBLGEN_VALUE_H
