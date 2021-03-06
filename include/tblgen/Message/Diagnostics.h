
#ifndef TBLGEN_DIAGNOSTICS_H
#define TBLGEN_DIAGNOSTICS_H

#include "tblgen/Lex/SourceLocation.h"

#include <stack>
#include <string>
#include <unordered_map>
#include <vector>

namespace tblgen {

struct SourceLocation;
class DiagnosticsEngine;

namespace lex {
   class Lexer;
} // namespace lex

namespace diag {

enum class SeverityLevel {
   Note,
   Warning,
   Error,
   Fatal,
};

namespace opt {
   enum Option {
      whole_line,
      show_wiggle,
      no_inst_ctx,
      no_expansion_info,
      no_import_info,
      memberwise_init,
      show_constness,
   };
} // namespace opt

struct FakeSourceLocation {
   mutable std::string text;
};

enum MessageKind : unsigned {
#  define TBLGEN_MSG(Name, Str)               \
   Name,
#  include "tblgen/Message/def/Diagnostics.def"
};

inline bool isWarning(MessageKind msg)
{
   return msg > _first_warn && msg < _last_warn;
}

inline bool isNote(MessageKind msg)
{
   return msg > _first_note && msg < _last_note;
}

inline bool isError(MessageKind msg)
{
   return msg > _first_err && msg < _last_err;
}

class DiagnosticBuilder {
public:
   explicit DiagnosticBuilder(DiagnosticsEngine &Engine);
   DiagnosticBuilder(DiagnosticsEngine &Engine, MessageKind kind);

   ~DiagnosticBuilder();

   // non-terminating
   DiagnosticBuilder& operator<<(std::string const& str);
   DiagnosticBuilder& operator<<(std::string_view str);
   DiagnosticBuilder& operator<<(const char* str);

   DiagnosticBuilder& operator<<(int i);
   DiagnosticBuilder& operator<<(uint64_t i);
   DiagnosticBuilder& operator<<(int64_t i);

   DiagnosticBuilder& operator<<(SourceLocation loc);
   DiagnosticBuilder& operator<<(SourceRange loc);
   DiagnosticBuilder& operator<<(FakeSourceLocation const& loc);

   DiagnosticBuilder& operator<<(opt::Option const& opt);

   bool isValid() const { return valid; }
   MessageKind getMessageKind() const { return msg; }

   operator bool() const { return isValid(); }

   void setLoc(SourceLocation l) const
   {
      this->loc = SourceRange(l);
   }

   void setLoc(SourceRange l) const
   {
      this->loc = l;
   }

   void disable() { Disabled = true; }

protected:
   void finalize();
   std::string prepareMessage(std::string_view str);
   void handleFunction(unsigned idx, lex::Lexer& lex,
                       std::string &msg);

   void appendArgumentString(unsigned idx, std::string &str);

   DiagnosticsEngine &Engine;

   mutable SourceRange loc;
   MessageKind msg;

   bool showWiggle : 1;
   bool showWholeLine : 1;
   bool noInstCtx : 1;
   bool noteMemberwiseInit : 1;
   bool valid : 1;
   bool noExpansionInfo : 1;
   bool noImportInfo : 1;
   bool hasFakeSourceLoc : 1;
   bool ShowConst : 1;
   bool Disabled : 1;
};

} // namespace diag
} // namespace tblgen


#endif //TBLGEN_DIAGNOSTICS_H
