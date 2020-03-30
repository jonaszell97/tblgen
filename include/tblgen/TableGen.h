
#ifndef TBLGEN_TABLEGEN_H
#define TBLGEN_TABLEGEN_H

#include "tblgen/Basic/IdentifierInfo.h"
#include "tblgen/Lex/SourceLocation.h"
#include "tblgen/Support/Allocator.h"
#include "tblgen/Type.h"

#include <unordered_map>

#define unreachable(MSG) assert(false && MSG); __builtin_unreachable()

namespace tblgen {

namespace fs {
   class FileManager;
} // namespace fs

class DiagnosticsEngine;
class Record;
class RecordKeeper;
class Class;

using TableGenBackend = void(std::ostream&, RecordKeeper&);

class TableGen {
public:
   TableGen(support::ArenaAllocator &Allocator, fs::FileManager &fileMgr,
            DiagnosticsEngine &Diags);

   void *Allocate(size_t size, size_t alignment = 8) const
   {
      return Allocator.Allocate(size, alignment);
   }

   template <typename T>
   T *Allocate(size_t Num = 1) const
   {
      return static_cast<T *>(Allocate(Num * sizeof(T), alignof(T)));
   }

   void Deallocate(void *Ptr) const {}

   support::ArenaAllocator &getAllocator() const
   {
      return Allocator;
   }

   IdentifierTable &getIdents() const
   {
      return Idents;
   }

   enum RecordFinalizeStatus {
      RFS_Success,
      RFS_MissingFieldValue,
      RFS_DuplicateField,
   };

   struct FinalizeResult {
      RecordFinalizeStatus status;
      std::string missingOrDuplicateFieldName;
      SourceLocation declLoc;
   };

   FinalizeResult finalizeRecord(Record &R);

   support::ArenaAllocator &Allocator;
   fs::FileManager &fileMgr;
   DiagnosticsEngine &Diags;
   std::unique_ptr<RecordKeeper> GlobalRK;

private:
   mutable IdentifierTable Idents;

   mutable IntType Int1Ty;
   mutable IntType Int8Ty;
   mutable IntType UInt8Ty;
   mutable IntType Int16Ty;
   mutable IntType UInt16Ty;
   mutable IntType Int32Ty;
   mutable IntType UInt32Ty;
   mutable IntType Int64Ty;
   mutable IntType UInt64Ty;

   mutable FloatType FloatTy;
   mutable DoubleType DoubleTy;

   mutable StringType StringTy;
   mutable CodeType CodeTy;

   mutable std::unordered_map<Class*, ClassType>   ClassTypes;
   mutable std::unordered_map<Record*, RecordType> RecordTypes;
   mutable std::unordered_map<Enum*, EnumType>     EnumTypes;
   mutable std::unordered_map<Type*, ListType>     ListTypes;
   mutable std::unordered_map<Type*, DictType>     DictTypes;

public:
   IntType *getInt1Ty() const { return &Int1Ty; }

   IntType *getInt8Ty() const { return &Int8Ty; }
   IntType *getUInt8Ty() const { return &UInt8Ty; }

   IntType *getInt16Ty() const { return &Int16Ty; }
   IntType *getUInt16Ty() const { return &UInt16Ty; }

   IntType *getInt32Ty() const { return &Int32Ty; }
   IntType *getUInt32Ty() const { return &UInt32Ty; }

   IntType *getInt64Ty() const { return &Int64Ty; }
   IntType *getUInt64Ty() const { return &UInt64Ty; }

   IntType *getIntegerTy(unsigned bits, bool isUnsigned) const
   {
      if (!isUnsigned) {
         switch (bits) {
         case 1: return &Int1Ty;
         case 8: return &Int8Ty;
         case 16: return &Int16Ty;
         case 32: return &Int32Ty;
         case 64: return &Int64Ty;
         default: unreachable("bad bitwidth");
         }
      }
      else {
         switch (bits) {
         case 1: return &Int1Ty;
         case 8: return &UInt8Ty;
         case 16: return &UInt16Ty;
         case 32: return &UInt32Ty;
         case 64: return &UInt64Ty;
         default: unreachable("bad bitwidth");
         }
      }
   }

   FloatType *getFloatTy() const { return &FloatTy; }
   DoubleType *getDoubleTy() const { return &DoubleTy; }

   StringType *getStringTy() const { return &StringTy; }
   CodeType *getCodeTy() const { return &CodeTy; }

   ClassType *getClassType(Class *C) const
   {
      auto it = ClassTypes.find(C);
      if (it != ClassTypes.end())
         return &it->second;

      return &ClassTypes.try_emplace(C, ClassType(C)).first->second;
   }

   RecordType *getRecordType(Record *R) const
   {
      auto it = RecordTypes.find(R);
      if (it != RecordTypes.end())
         return &it->second;

      return &RecordTypes.try_emplace(R, RecordType(R)).first->second;
   }

   EnumType *getEnumType(Enum *E) const
   {
      auto it = EnumTypes.find(E);
      if (it != EnumTypes.end())
         return &it->second;

      return &EnumTypes.try_emplace(E, EnumType(E)).first->second;
   }

   ListType *getListType(Type *ElementTy) const
   {
      auto it = ListTypes.find(ElementTy);
      if (it != ListTypes.end())
         return &it->second;

      return &ListTypes.try_emplace(ElementTy,
                                    ListType(ElementTy)).first->second;
   }

   DictType *getDictType(Type *ElementTy)
   {
      auto it = DictTypes.find(ElementTy);
      if (it != DictTypes.end())
         return &it->second;

      return &DictTypes.try_emplace(ElementTy,
                                    DictType(ElementTy)).first->second;
   }
};

} // namespace tblgen

inline void *operator new(size_t size, ::tblgen::TableGen const& TG,
                          size_t alignment = 8) {
   return TG.Allocate(size, alignment);
}

inline void operator delete(void *ptr, ::tblgen::TableGen const& TG,
                            size_t) {
   return TG.Deallocate(ptr);
}

inline void *operator new[](size_t size, ::tblgen::TableGen const& TG,
                            size_t alignment = 8) {
   return TG.Allocate(size, alignment);
}

inline void operator delete[](void *ptr, ::tblgen::TableGen const& TG,
                              size_t) {
   return TG.Deallocate(ptr);
}

#endif //TBLGEN_TABLEGEN_H
