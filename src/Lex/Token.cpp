//
// Created by Jonas Zell on 13.06.17.
//

#include "tblgen/Lex/Token.h"

#include "tblgen/Basic/IdentifierInfo.h"
#include "tblgen/Support/Format.h"

#include <llvm/ADT/APSInt.h>
#include <llvm/ADT/SmallString.h>
#include <llvm/ADT/FoldingSet.h>
#include <llvm/Support/raw_ostream.h>

using tblgen::lex::tok::TokenType;
using std::string;

namespace tblgen {
namespace lex {
namespace tok {

string tokenTypeToString(TokenType kind)
{
   switch (kind) {
   case tok::ident:
      return "identifier";
   case tok::op_ident:
      return "operator";
   case tok::dollar_ident:
      return "$-identifier";
   case tok::dollar_dollar_ident:
      return "$$-identifier";
   case tok::percent_ident:
      return "%-identifier";
   case tok::percent_percent_ident:
      return "%%-identifier";
   case tok::charliteral:
      return "character literal";
   case tok::stringliteral:
      return "string literal";
   case tok::fpliteral:
      return "floating point literal";
   case tok::integerliteral:
      return "integer literal";
   default:
      return Token(kind).toString();
   }
}

} // namespace tok

template <unsigned StrLen>
static constexpr unsigned strLen(const char (&Str)[StrLen])
{
   return StrLen;
}

void Token::Profile(llvm::FoldingSetNodeID &ID) const
{
   ID.AddInteger(kind);
   ID.AddPointer(Ptr);
   ID.AddInteger(Data);
   ID.AddInteger(loc.getOffset());
}

SourceLocation Token::getEndLoc() const
{
   unsigned Length;
   switch (kind) {
   case tok::charliteral:
   case tok::stringliteral:
   case tok::integerliteral:
   case tok::fpliteral:
      Length = Data;
      break;
#  define TBLGEN_OPERATOR_TOKEN(Name, Spelling)             \
   case tok::Name: Length = strLen(Spelling); break;
#  define TBLGEN_KEYWORD_TOKEN(Name, Spelling)              \
   case tok::Name: Length = strLen(Spelling); break;
#  define TBLGEN_POUND_KEYWORD_TOKEN(Name, Spelling)        \
   case tok::Name: Length = strLen(Spelling); break;
#  define TBLGEN_PUNCTUATOR_TOKEN(Name, Spelling)           \
   case tok::Name: Length = 1; break;
#  include "tblgen/Lex/Tokens.def"
   case tok::sentinel:
   case tok::eof:
   case tok::interpolation_begin: Length = 2; break;
   case tok::interpolation_end: Length = 1; break;
   case tok::ident:
   case tok::op_ident:
   case tok::macro_name:
      Length = (unsigned)getIdentifierInfo()->getIdentifier().size();
      break;
   default:
      unreachable("bad token kind");
   }

   return SourceLocation(loc.getOffset() + Length - 1);
}

template<unsigned N>
void Token::rawRepr(llvm::SmallString<N> &s) const
{
   if (kind == tok::space) {
      s += getText();
      return;
   }

   switch (kind) {
   case tok::charliteral:
   case tok::stringliteral:
      s += std::string_view(reinterpret_cast<const char*>(Ptr) - 1,
                           Data + 2);
      break;
#  define TBLGEN_PUNCTUATOR_TOKEN(Name, Spelling)                               \
   case tok::Name: s += (Spelling); break;
#  include "tblgen/Lex/Tokens.def"
   default:
      toString(s);
      break;
   }
}

template<unsigned N>
void Token::toString(llvm::SmallString<N> &s) const
{
   llvm::raw_svector_ostream OS(s);
   print(OS);
}

void Token::dump() const
{
   print(std::cerr);
}

void Token::print(llvm::raw_ostream &OS) const
{
   if (kind == tok::space) {
      OS << getText();
      return;
   }

   switch (kind) {
#  define TBLGEN_OPERATOR_TOKEN(Name, Spelling)                               \
   case tok::Name: OS << (Spelling); return;
#  define TBLGEN_KEYWORD_TOKEN(Name, Spelling)                                \
   case tok::Name: OS << (Spelling); return;
#  define TBLGEN_POUND_KEYWORD_TOKEN(Name, Spelling)                          \
   case tok::Name: OS << (Spelling); return;
#  define TBLGEN_CONTEXTUAL_KW_TOKEN(Name, Spelling)                          \
   case tok::Name: OS << (Spelling); return;
#  define TBLGEN_PUNCTUATOR_TOKEN(Name, Spelling)                             \
   case tok::Name: support::unescape_char((Spelling), OS); return;
#  define TBLGEN_TABLEGEN_KW_TOKEN(Name, Spelling) TBLGEN_KEYWORD_TOKEN(Name, Spelling)
   case tok::sentinel: OS << "<sentinel>"; return;
   case tok::eof: OS << "<eof>"; return;
   case tok::expr_begin: OS << "#{"; return;
   case tok::stringify_begin: OS << "##{"; return;
   case tok::interpolation_begin: OS << "${"; return;
   case tok::interpolation_end: OS << "}"; return;
   case tok::ident:
   case tok::op_ident:
      OS << getIdentifier(); return;
   case tok::dollar_ident: {
      OS << "$" << getIdentifier();
      return;
   }
   case tok::charliteral:
   case tok::stringliteral:
      OS << std::string_view(reinterpret_cast<const char*>(Ptr) - 1, Data + 2);
      break;
   case tok::fpliteral:
   case tok::integerliteral:
      OS << getText();
      return;
   case tok::macro_name:
      OS << getIdentifier() << "!";
      break;
   case tok::macro_expression:
      OS << "<expr " << getExpr() << ">";
      break;
   case tok::macro_statement:
      OS << "<stmt " << getStmt() << ">";
      break;
   case tok::macro_declaration:
      OS << "<decl " << getDecl() << ">";
      break;
   default:
      unreachable("unhandled token kind");

#  include "tblgen/Lex/Tokens.def"
   }
}

template void Token::toString(llvm::SmallString<64>&) const;
template void Token::toString(llvm::SmallString<128>&) const;
template void Token::toString(llvm::SmallString<256>&) const;
template void Token::toString(llvm::SmallString<512>&) const;

template void Token::rawRepr(llvm::SmallString<64>&) const;
template void Token::rawRepr(llvm::SmallString<128>&) const;
template void Token::rawRepr(llvm::SmallString<256>&) const;
template void Token::rawRepr(llvm::SmallString<512>&) const;

bool Token::is(tblgen::IdentifierInfo *II) const
{
   if (!is_identifier())
      return false;

   return getIdentifierInfo() == II;
}

bool Token::isIdentifier(std::string_view str) const
{
   if (!is_identifier())
      return false;

   return getIdentifierInfo()->getIdentifier().equals(str);
}

bool Token::isIdentifierStartingWith(std::string_view str) const
{
   if (!is_identifier())
      return false;

   return getIdentifierInfo()->getIdentifier().startswith(str);
}

bool Token::isIdentifierEndingWith(std::string_view str) const
{
   if (!is_identifier())
      return false;

   return getIdentifierInfo()->getIdentifier().endswith(str);
}

bool Token::isWhitespace() const
{
   switch (getKind()) {
   case tok::space: case tok::newline:
      return true;
   default:
      return false;
   }
}

std::string_view Token::getIdentifier() const
{
   return getIdentifierInfo()->getIdentifier();
}

llvm::APInt Token::getIntegerValue() const
{
   assert(kind == tok::integerliteral);

   auto txt = getText();
   uint8_t base = 10;
   unsigned offset = 0;

   if (txt[0] == '0') {
      if (txt.size() > 1) {
         if (txt[1] == 'x' || txt[1] == 'X') {
            offset = 2;
            base = 16;
         }
         else if (txt[1] == 'b' || txt[1] == 'B') {
            offset = 2;
            base = 2;
         }
         else {
            offset = 1;
            base = 8;
         }
      }
   }

   std::string_view str(txt.data() + offset, txt.size() - offset);
   return llvm::APInt(64, str, base);
}

string Token::toString() const
{
   llvm::SmallString<128> str;
   toString(str);

   return str.str();
}

string Token::rawRepr() const
{
   llvm::SmallString<128> str;
   rawRepr(str);

   return str.str();
}

bool Token::is_punctuator() const
{
   switch (kind) {
#  define TBLGEN_PUNCTUATOR_TOKEN(Name, Spelling) \
      case tok::Name:
#  include "tblgen/Lex/Tokens.def"
         return true;
      default:
         return false;
   }
}

bool Token::is_keyword() const
{
   switch (kind) {
#  define TBLGEN_KEYWORD_TOKEN(Name, Spelling) \
      case tok::Name:
#  define TBLGEN_MODULE_KEYWORD_TOKEN(Name, Spelling) \
      case tok::Name:
#  define TBLGEN_TABLEGEN_KW_TOKEN(Name, Spelling) TBLGEN_KEYWORD_TOKEN(Name, Spelling)
#  include "tblgen/Lex/Tokens.def"
         return true;
      default:
         return false;
   }
}

bool Token::is_operator() const
{
   switch (kind) {
#  define TBLGEN_OPERATOR_TOKEN(Name, Spelling) \
      case tok::Name:
#  include "tblgen/Lex/Tokens.def"
         return true;
      default:
         return false;
   }
}

bool Token::is_directive() const
{
   switch (kind) {
#  define TBLGEN_POUND_KEYWORD_TOKEN(Name, Spelling) \
      case tok::Name:
#  include "tblgen/Lex/Tokens.def"
         return true;
      default:
         return false;
   }
}

bool Token::is_literal() const
{
   switch (kind) {
#  define TBLGEN_LITERAL_TOKEN(Name, Spelling) \
      case tok::Name:
#  include "tblgen/Lex/Tokens.def"
         return true;
      default:
         return false;
   }
}

} // namespace lex
} // namespace tblgen