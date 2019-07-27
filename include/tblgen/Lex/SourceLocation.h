//
// Created by Jonas Zell on 16.11.17.
//

#ifndef TBLGEN_SOURCELOCATION_H
#define TBLGEN_SOURCELOCATION_H

#include "tblgen/Support/Config.h"

#include <cassert>
#include <cstdint>

namespace tblgen {

struct SourceLocation {
   TBLGEN_LLDB_STEP_OVER
   SourceLocation() : offset(0)  {}

   TBLGEN_LLDB_STEP_OVER
   explicit SourceLocation(uint32_t offset) : offset(offset)
   {

   }

private:
   uint32_t offset;

public:
   uint32_t getOffset() const { return offset; }

   bool isValid() const { return offset > 0; }
   operator bool() const { return isValid(); }

   friend bool operator==(const SourceLocation &LHS, const SourceLocation &RHS)
   {
      return LHS.offset == RHS.offset;
   }

   friend bool operator!=(const SourceLocation &LHS, const SourceLocation &RHS)
   {
      return LHS.offset != RHS.offset;
   }

   friend bool operator<(const SourceLocation &LHS, const SourceLocation &RHS)
   {
      return LHS.offset < RHS.offset;
   }

   friend bool operator<=(const SourceLocation &LHS, const SourceLocation &RHS)
   {
      return LHS.offset <= RHS.offset;
   }

   friend bool operator>(const SourceLocation &LHS, const SourceLocation &RHS)
   {
      return LHS.offset > RHS.offset;
   }

   friend bool operator>=(const SourceLocation &LHS, const SourceLocation &RHS)
   {
      return LHS.offset >= RHS.offset;
   }
};

struct SourceRange {
   TBLGEN_LLDB_STEP_OVER
   SourceRange() = default;

   TBLGEN_LLDB_STEP_OVER
   /*implicit*/ SourceRange(SourceLocation start)
      : start(start), end(SourceLocation())
   {}

   TBLGEN_LLDB_STEP_OVER
   SourceRange(SourceLocation start, SourceLocation end)
      : start(start), end(end)
   {}

   SourceLocation getStart() const
   {
      return start;
   }

   SourceLocation getEnd() const
   {
      return end;
   }

   operator bool() const { return start.isValid() && end.isValid(); }

   friend bool operator==(const SourceRange &LHS, const SourceRange &RHS)
   {
      return LHS.start == RHS.start && LHS.end == RHS.end;
   }

   friend bool operator!=(const SourceRange &LHS, const SourceRange &RHS)
   {
      return !(LHS == RHS);
   }

private:
   SourceLocation start;
   SourceLocation end;
};

} // namespace tblgen

namespace llvm {

template <typename T> struct PointerLikeTypeTraits;
template<class T> struct DenseMapInfo;

template<>
struct PointerLikeTypeTraits<::tblgen::SourceLocation> {
public:
   static inline void *getAsVoidPointer(::tblgen::SourceLocation P)
   {
      return reinterpret_cast<void*>(P.getOffset());
   }

   static inline ::tblgen::SourceLocation getFromVoidPointer(void *P)
   {
      return ::tblgen::SourceLocation(
         static_cast<uint32_t>(reinterpret_cast<uint64_t>(P)));
   }

   enum { NumLowBitsAvailable = 0 };
};

template<> struct DenseMapInfo<::tblgen::SourceLocation> {
   static ::tblgen::SourceLocation getEmptyKey()
   {
      return ::tblgen::SourceLocation(static_cast<uint32_t>(-1));
   }

   static ::tblgen::SourceLocation getTombstoneKey()
   {
      return ::tblgen::SourceLocation(static_cast<uint32_t>(-2));
   }

   static int getHashValue(const ::tblgen::SourceLocation P)
   {
      return static_cast<int>(P.getOffset());
   }

   static bool isEqual(const ::tblgen::SourceLocation LHS,
                       const ::tblgen::SourceLocation RHS) {
      return LHS == RHS;
   }
};

} // namespace llvm


#endif //TBLGEN_SOURCELOCATION_H
