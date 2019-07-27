//
// Created by Jonas Zell on 04.10.17.
//

#include "tblgen/Message/Diagnostics.h"

#include "tblgen/Basic/IdentifierInfo.h"
#include "tblgen/Basic/FileManager.h"
#include "tblgen/Basic/FileUtils.h"
#include "tblgen/Lex/Lexer.h"
#include "tblgen/Lex/Token.h"
#include "tblgen/Message/DiagnosticsEngine.h"
#include "tblgen/Support/Casting.h"
#include "tblgen/Support/Format.h"

#include <llvm/ADT/APInt.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/Support/MemoryBuffer.h>

#include <cassert>
#include <cstdlib>
#include <sstream>

using namespace tblgen::lex;
using namespace tblgen::ast;
using namespace tblgen::support;

using std::string;

namespace tblgen {
namespace diag {

static llvm::StringRef getMessage(MessageKind msg)
{
   switch (msg) {
#  define TBLGEN_MSG(Name, Msg)                   \
      case Name: return #Msg;
#  include "tblgen/Message/def/Diagnostics.def"
   }

   unreachable("bad msg kind");
}

DiagnosticBuilder::DiagnosticBuilder(DiagnosticsEngine &Engine)
   : Engine(Engine), msg(_first_err), showWiggle(false), showWholeLine(false),
     noInstCtx(false), noteMemberwiseInit(false), valid(false),
     noExpansionInfo(false), noImportInfo(false), hasFakeSourceLoc(false),
     ShowConst(false), Disabled(false)
{
   assert(!Engine.hasInFlightDiag() && "diagnostic issued while preparing "
                                       "other diagnostic!");
}

DiagnosticBuilder::DiagnosticBuilder(DiagnosticsEngine &Engine,
                                     MessageKind msg)
   : Engine(Engine), msg(msg), showWiggle(false), showWholeLine(false),
     noInstCtx(false), noteMemberwiseInit(false), valid(true),
     noExpansionInfo(false), noImportInfo(false), hasFakeSourceLoc(false),
     ShowConst(false), Disabled(false)
{
   assert(!Engine.hasInFlightDiag() && "diagnostic issued while preparing "
                                       "other diagnostic!");
}

DiagnosticBuilder::~DiagnosticBuilder()
{
   if (Disabled) {
      Engine.NumArgs = 0;
      Engine.NumSourceRanges = 0;

      return;
   }

   finalize();
}

string DiagnosticBuilder::prepareMessage(llvm::StringRef str)
{
   auto buf = llvm::MemoryBuffer::getMemBuffer(str);

   IdentifierTable IT(16);

   Lexer lex(IT, Engine, buf.get(), 0, 1, '$', false);
   lex.lexDiagnostic();

   unsigned short substituted = 0;
   std::string msg = "";
   bool single = true;

   while (!lex.currentTok().is(tok::eof)) {
      single = false;
      if (lex.currentTok().is(tok::sentinel)) {
         lex.advance();

         if (lex.currentTok().isIdentifier("$")) {
            msg += "$";
            lex.advance();

            continue;
         }

         ++substituted;

         if (lex.lookahead().is(tok::sentinel)) {
            assert(lex.currentTok().is(tok::integerliteral)
                   && "expected arg index");

            auto txt = lex.currentTok().getText();
            auto val = std::stoul(txt);

            appendArgumentString(val, msg);
         }
         else {
            assert(lex.currentTok().is(tok::integerliteral)
                   && "expected arg index");

            auto txt = lex.currentTok().getText();
            auto val = std::stoull(txt);
            assert(Engine.NumArgs > val && "not enough args provided");

            lex.advance();
            assert(lex.currentTok().is(tok::op_or) && "expected pipe");

            lex.advance();
            handleFunction(val, lex, msg);
         }

         lex.advance();
         assert(lex.currentTok().is(tok::sentinel));
      }
      else {
         msg += lex.currentTok().getText();
      }

      lex.advance();
   }

   if (single) {
      msg += lex.currentTok().getText();
   }

   return msg;
}

void DiagnosticBuilder::appendArgumentString(unsigned idx, std::string &str)
{
   auto kind = Engine.ArgKinds[idx];
   switch (kind) {
   case DiagnosticsEngine::ak_string:
      str += Engine.StringArgs[idx];
      break;
   case DiagnosticsEngine::ak_integer:
      str += std::to_string(Engine.OtherArgs[idx]);
      break;
   default:
      unreachable("unhandled argument kind");
   }
}

void DiagnosticBuilder::handleFunction(unsigned idx, lex::Lexer& lex,
                                       std::string &msg) {
   llvm::StringRef funcName;
   if (lex.currentTok().is_keyword()) {
      switch (lex.currentTok().getKind()) {
         case tok::kw_if:
            funcName = "if";
            break;
         default:
            unreachable("unexpected keyword in diagnostic");
      }
   }
   else {
      assert(lex.currentTok().is(tok::ident));
      funcName = lex.getCurrentIdentifier();
   }

   std::vector<string> args;

   if (lex.lookahead().is(tok::open_paren)) {
      lex.advance();
      lex.advance();

      while (!lex.currentTok().is(tok::close_paren)) {
         assert(lex.currentTok().is(tok::stringliteral));

         args.emplace_back(lex.currentTok().getText());
         lex.advance();
      }
   }

   if (funcName == "select") {
      unsigned val;
      if (Engine.ArgKinds[idx] == DiagnosticsEngine::ak_integer) {
         val = (unsigned) Engine.OtherArgs[idx];
      }
      else if (Engine.ArgKinds[idx] == DiagnosticsEngine::ak_string) {
         val = (unsigned)!Engine.StringArgs[idx].empty();
      }
      else {
         unreachable("bad arg kind");
      }

      assert(args.size() > val && "too few options for index");

      auto &str = args[val];
      msg += prepareMessage(str);
   }
   else if (funcName == "ordinal") {
      assert(args.empty() && "ordinal takes no arguments");
      assert(Engine.ArgKinds[idx] == DiagnosticsEngine::ak_integer);

      auto val = Engine.OtherArgs[idx];
      auto mod = val % 10;
      msg += std::to_string(val);

      switch (mod) {
         case 1: msg += "st"; break;
         case 2: msg += "nd"; break;
         case 3: msg += "rd"; break;
         default: msg += "th"; break;
      }
   }
   else if (funcName == "plural_s") {
      assert(args.size() == 1 && "plural_s takes 1 argument");
      assert(Engine.ArgKinds[idx] == DiagnosticsEngine::ak_integer);

      auto val = Engine.OtherArgs[idx];
      if (val != 1) {
         msg += args.front() + "s";
      }
      else {
         msg += args.front();
      }
   }
   else if (funcName == "plural") {
      assert(!args.empty() && "plural expects at least 1 argument");
      assert(Engine.ArgKinds[idx] == DiagnosticsEngine::ak_integer);

      auto val = Engine.OtherArgs[idx];
      if (val >= args.size()) {
         msg += args.back();
      }
      else {
         msg += args[val];
      }
   }
   else if (funcName == "if") {
      assert(args.size() == 1 && "if expects 1 arg");

      if (Engine.ArgKinds[idx] == DiagnosticsEngine::ak_integer) {
         auto val = Engine.OtherArgs[idx];
         if (val) {
            msg += prepareMessage(args.front());
         }
      }
      else if (Engine.ArgKinds[idx] == DiagnosticsEngine::ak_string) {
         auto val = Engine.StringArgs[idx];
         if (!val.empty()) {
            msg += prepareMessage(args.front());
         }
      }
      else {
         unreachable("bad arg kind");
      }
   }
}

static SeverityLevel getSeverity(MessageKind msg)
{
   switch (msg) {
#  define TBLGEN_WARN(Name, Msg)                                               \
   case Name: return SeverityLevel::Warning;
#  define TBLGEN_NOTE(Name, Msg)                                               \
   case Name: return SeverityLevel::Note;
#  define TBLGEN_ERROR(Name, Msg, IsFatal)                                     \
   case Name: return IsFatal ? SeverityLevel::Fatal : SeverityLevel::Error;
#  include "tblgen/Message/def/Diagnostics.def"
   }
}

static bool isNewline(const char *str)
{
   switch (str[0]) {
   case '\n': case '\r':
      return true;
   default:
      return false;
   }
}

void DiagnosticBuilder::finalize()
{
   std::string str;
   llvm::raw_string_ostream out(str);

   auto severity = getSeverity(msg);
   switch (severity) {
   case SeverityLevel::Error: out << "\033[21;31merror:\033[0m ";
      break;
   case SeverityLevel::Fatal: out << "\033[21;31mfatal error:\033[0m ";
      break;
   case SeverityLevel::Warning: out << "\033[33mwarning:\033[0m ";
      break;
   case SeverityLevel::Note: out << "\033[1;35mnote:\033[0m ";
      break;
   }

   out << prepareMessage(getMessage(msg));

   if (hasFakeSourceLoc) {
      out << "\n" << Engine.StringArgs[Engine.NumArgs - 1] << "\n\n";
      out.flush();
      Engine.finalizeDiag(str, severity);

      return;
   }

   if (!Engine.NumSourceRanges) {
      out << "\n";
      out.flush();
      Engine.finalizeDiag(str, severity);

      return;
   }

   const IdentifierInfo *MacroName = nullptr;
   SourceLocation ExpandedFromLoc;

   SourceLocation loc = Engine.SourceRanges[0].getStart();

   size_t ID = Engine.FileMgr->getSourceId(loc);
   auto File = Engine.FileMgr->getOpenedFile(ID);

   llvm::MemoryBuffer *Buf = File.Buf;
   size_t srcLen = Buf->getBufferSize();
   const char *src = Buf->getBufferStart();

   // show file name, line number and column
   auto lineAndCol = Engine.FileMgr->getLineAndCol(loc, Buf);
   out << " (" << fs::getFileNameAndExtension(File.FileName)
       << ":" << lineAndCol.line << ":" << lineAndCol.col << ")\n";

   unsigned errLineNo = lineAndCol.line;

   // only source ranges that are on the same line as the "main index" are shown
   unsigned errIndex     = loc.getOffset() - File.BaseOffset;
   unsigned newlineIndex = errIndex;

   // find offset of first newline before the error index
   for (; newlineIndex > 0; --newlineIndex) {
      if (isNewline(src + newlineIndex))
         break;
   }

   // find offset of first newline after error index
   unsigned lineEndIndex = errIndex;
   for (; lineEndIndex < srcLen; ++lineEndIndex) {
      if (isNewline(src + lineEndIndex))
         break;
   }

   unsigned Len;
   if (lineEndIndex == newlineIndex) {
      Len = 0;
   }
   else {
      Len = lineEndIndex - newlineIndex - 1;
   }

   llvm::StringRef ErrLine(Buf->getBufferStart() + newlineIndex + 1, Len);

   // show carets for any given single source location, and tildes for source
   // ranges (but only on the error line)
   std::string Markers;

   Markers.resize(ErrLine.size());
   std::fill(Markers.begin(), Markers.end(), ' ');

   for (unsigned i = 0; i < Engine.NumSourceRanges; ++i) {
      auto &SR = Engine.SourceRanges[i];
      if (!SR.getStart())
         continue;

      auto Start = SR.getStart();
      auto End   = SR.getEnd();

      assert(Engine.FileMgr->getSourceId(Start) == ID
             && "source locations in different files!");

      // single source location, show caret
      if (!End || End == Start) {
         unsigned offset = Start.getOffset() - File.BaseOffset;
         if (lineEndIndex <= offset || newlineIndex >= offset) {
            // source location is on a different line
            continue;
         }

         unsigned offsetOnLine = offset - newlineIndex - 1;
         Markers[offsetOnLine] = '^';
      }
      else {
         auto BeginOffset = Start.getOffset() - File.BaseOffset;
         auto EndOffset   = End.getOffset() - File.BaseOffset;

         unsigned BeginOffsetOnLine = BeginOffset - newlineIndex - 1;
         unsigned EndOffsetOnLine   = std::min(EndOffset, lineEndIndex)
                                      - newlineIndex - 1;

         if (EndOffsetOnLine > lineEndIndex)
            continue;

         assert(EndOffsetOnLine >= BeginOffsetOnLine
                && "invalid source range!");

         while (1) {
            Markers[EndOffsetOnLine] = '~';
            if (EndOffsetOnLine-- == BeginOffsetOnLine)
               break;
         }

         // If there's only one location and it's a range, show a caret
         // instead of the first tilde
         if (Engine.NumSourceRanges == 1) {
            Markers[BeginOffsetOnLine] = '^';
         }
      }
   }

   // display line number to the left of the source
   size_t LinePrefixSize = out.GetNumBytesInBuffer();
   out << errLineNo << " | ";

   LinePrefixSize = out.GetNumBytesInBuffer() - LinePrefixSize;
   out << ErrLine << "\n"
       << std::string(LinePrefixSize, ' ') << Markers << "\n";

   Engine.finalizeDiag(out.str(), severity);

   if ((int)severity >= (int)SeverityLevel::Error && ExpandedFromLoc) {
      DiagnosticBuilder(Engine, diag::note_in_expansion)
         << ExpandedFromLoc
         << MacroName->getIdentifier();
   }
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(int i)
{
   return *this << (size_t)i;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(size_t i)
{
   Engine.ArgKinds[Engine.NumArgs] = DiagnosticsEngine::ak_integer;
   Engine.OtherArgs[Engine.NumArgs] = i;
   ++Engine.NumArgs;

   return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(llvm::APInt const &API)
{
   Engine.ArgKinds[Engine.NumArgs] = DiagnosticsEngine::ak_string;
   Engine.StringArgs[Engine.NumArgs] = API.toString(10, true);
   ++Engine.NumArgs;

   return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(string const& str)
{
   Engine.ArgKinds[Engine.NumArgs] = DiagnosticsEngine::ak_string;
   Engine.StringArgs[Engine.NumArgs] = str;
   ++Engine.NumArgs;

   return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(llvm::Twine const &str)
{
   Engine.ArgKinds[Engine.NumArgs] = DiagnosticsEngine::ak_string;
   Engine.StringArgs[Engine.NumArgs] = str.str();
   ++Engine.NumArgs;

   return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(llvm::StringRef str)
{
   Engine.ArgKinds[Engine.NumArgs] = DiagnosticsEngine::ak_string;
   Engine.StringArgs[Engine.NumArgs] = str.str();
   ++Engine.NumArgs;

   return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(const char* str)
{
   Engine.ArgKinds[Engine.NumArgs] = DiagnosticsEngine::ak_string;
   Engine.StringArgs[Engine.NumArgs] = str;
   ++Engine.NumArgs;

   return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(SourceLocation loc)
{
   if (loc)
      Engine.SourceRanges[Engine.NumSourceRanges++] = SourceRange(loc);

   return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(SourceRange loc)
{
   if (loc.getStart())
      Engine.SourceRanges[Engine.NumSourceRanges++] = loc;

   return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(FakeSourceLocation const& loc)
{
   hasFakeSourceLoc = true;

   Engine.ArgKinds[Engine.NumArgs] = DiagnosticsEngine::ak_string;
   Engine.StringArgs[Engine.NumArgs] = move(loc.text);
   ++Engine.NumArgs;

   return *this;
}

DiagnosticBuilder& DiagnosticBuilder::operator<<(opt::Option const &opt)
{
   using namespace opt;

   switch (opt) {
      case whole_line:
         showWholeLine = true;
         break;
      case show_wiggle:
         showWiggle = true;
         break;
      case no_inst_ctx:
         noInstCtx = true;
         break;
      case no_expansion_info:
         noExpansionInfo = true;
         break;
      case memberwise_init:
         noteMemberwiseInit = true;
         break;
      case no_import_info:
         noImportInfo = true;
         break;
      case show_constness:
         ShowConst = true;
         break;
   }

   return *this;
}

} // namespace diag
} // namespace tblgen