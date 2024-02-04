
#ifndef TBLGEN_TYPE_H
#define TBLGEN_TYPE_H

#include <iosfwd>
#include <string>

namespace tblgen {

class Enum;
class Class;
class Record;
class TableGen;

class Type {
public:
   enum TypeID {
      IntTypeID,
      FloatTypeID,
      DoubleTypeID,
      StringTypeID,
      CodeTypeID,
      ListTypeID,
      DictTypeID,
      ClassTypeID,
      RecordTypeID,
      EnumTypeID,
      UndefTypeID,
   };

   TypeID getTypeID() const
   {
      return typeID;
   }

   std::string toString() const;

protected:
   Type(TypeID typeID) : typeID(typeID)
   {}

#ifndef NDEBUG
   virtual
#endif
   ~Type() = default;

   TypeID typeID;
};

class IntType: public Type {
public:
   unsigned int getBitWidth() const
   {
      return BitWidth;
   }

   bool isUnsigned() const
   {
      return IsUnsigned;
   }

   static bool classof(Type const* T) { return T->getTypeID() == IntTypeID; }

   friend class TableGen;

private:
   IntType(unsigned int BitWidth, bool IsUnsigned)
      : Type(IntTypeID), BitWidth(BitWidth), IsUnsigned(IsUnsigned)
   { }

   unsigned BitWidth   : 7;
   bool     IsUnsigned : 1;
};

class FloatType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == FloatTypeID; }
   friend class TableGen;

private:
   FloatType() : Type(FloatTypeID)
   {}
};

class DoubleType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == DoubleTypeID; }
   friend class TableGen;

private:
   DoubleType() : Type(DoubleTypeID)
   {}
};

class StringType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == StringTypeID; }
   friend class TableGen;

private:
   StringType() : Type(StringTypeID)
   {}
};

class UndefType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == UndefTypeID; }
   friend class TableGen;

private:
   UndefType() : Type(UndefTypeID)
   {}
};

class CodeType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == CodeTypeID; }
   friend class TableGen;

private:
   CodeType() : Type(CodeTypeID)
   {}
};

class ListType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == ListTypeID; }
   friend class TableGen;

   Type *getElementType() const
   {
      return ElementType;
   }

private:
   ListType(Type *ElementType) : Type(ListTypeID), ElementType(ElementType)
   {}

   Type *ElementType;
};

class DictType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == DictTypeID; }
   friend class TableGen;

   Type *getElementType() const
   {
      return ElementType;
   }

private:
   DictType(Type *ElementType)
      : Type(DictTypeID), ElementType(ElementType)
   {}

   Type *ElementType;
};

class ClassType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == ClassTypeID; }
   friend class TableGen;

   Class *getClass() const
   {
      return C;
   }

private:
   ClassType(Class *C) : Type(ClassTypeID), C(C)
   {}

   Class *C;
};

class RecordType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == RecordTypeID; }
   friend class TableGen;

   Record *getRecord() const
   {
      return R;
   }

private:
   RecordType(Record *R) : Type(RecordTypeID), R(R)
   {}

   Record *R;
};

class EnumType: public Type {
public:
   static bool classof(Type const* T) { return T->getTypeID() == EnumTypeID; }
   friend class TableGen;

   Enum *getEnum() const
   {
      return E;
   }

private:
   EnumType(Enum *E) : Type(EnumTypeID), E(E)
   {}

   Enum *E;
};

std::ostream &operator<<(std::ostream &str, Type const* Ty);

} // namespace tblgen

#endif //TBLGEN_TYPE_H
