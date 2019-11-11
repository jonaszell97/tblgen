
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

#endif //TBLGEN_SOURCELOCATION_H
