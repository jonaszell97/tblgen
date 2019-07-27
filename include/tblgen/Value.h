//
// Created by Jonas Zell on 01.02.18.
//

#ifndef TBLGEN_VALUE_H
#define TBLGEN_VALUE_H

#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/Support/Casting.h>

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
   explicit IntegerLiteral(Type *Ty, llvm::APSInt &&Val)
      : Value(IntegerLiteralID, Ty), Val(std::move(Val))
   { }

   explicit IntegerLiteral(Type *Ty, llvm::APInt &&Val)
      : Value(IntegerLiteralID, Ty), Val(std::move(Val))
   { }

   const llvm::APSInt &getVal() const
   {
      return Val;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == IntegerLiteralID;}

private:
   llvm::APSInt Val;
};

class FPLiteral: public Value {
public:
   explicit FPLiteral(Type *Ty, llvm::APFloat &&Val)
      : Value(FPLiteralID, Ty), Val(std::move(Val))
   { }

   const llvm::APFloat &getVal() const
   {
      return Val;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == FPLiteralID;}

private:
   llvm::APFloat Val;
};

class StringLiteral: public Value {
public:
   explicit StringLiteral(Type *Ty, llvm::StringRef Val)
      : Value(StringLiteralID, Ty), Val(Val)
   { }

   llvm::StringRef getVal() const
   {
      return Val;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == StringLiteralID;}

private:
   llvm::StringRef Val;
};

class CodeBlock: public Value {
public:
   explicit CodeBlock(Type *Ty, std::string &&Val)
      : Value(CodeBlockID, Ty), Code(move(Val))
   { }

   llvm::StringRef getCode() const
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
         this->Values.try_emplace(llvm::cast<StringLiteral>(Keys[i])
                                     ->getVal(), Values[i]);
   }

   const llvm::StringMap<Value*> &getValues() const
   {
      return Values;
   }

   Value *getValue(llvm::StringRef key) const
   {
      auto it = Values.find(key);
      if (it != Values.end())
         return it->getValue();

      return nullptr;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == DictLiteralID;}

private:
   llvm::StringMap<Value*> Values;
};

class IdentifierVal: public Value {
public:
   explicit IdentifierVal(Type *Ty, llvm::StringRef Val)
      : Value(IdentifierValID, Ty), Val(Val)
   { }

   llvm::StringRef getVal() const
   {
      return Val;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == IdentifierValID;}

private:
   llvm::StringRef Val;
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
   explicit DictAccessExpr(Value *dict, llvm::StringRef key)
      : Value(DictAccessExprID, nullptr), dict(dict), key(key)
   {}

   Value *getDict() const
   {
      return dict;
   }

   const llvm::StringRef &getKey() const
   {
      return key;
   }

   static bool classof(Value const* V)
   { return V->getTypeID() == DictAccessExprID;}

private:
   Value *dict;
   llvm::StringRef key;
};

llvm::raw_ostream &operator<<(llvm::raw_ostream &str, Value const *V);

} // namespace tblgen

#endif //TBLGEN_VALUE_H
