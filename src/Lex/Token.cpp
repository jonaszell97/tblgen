
#include "tblgen/Lex/Token.h"

#include "tblgen/TableGen.h"
#include "tblgen/Basic/IdentifierInfo.h"
#include "tblgen/Support/Format.h"
#include "tblgen/Support/LiteralParser.h"

#include <iostream>
#include <sstream>

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

SourceLocation Token::getEndLoc() const
{
   unsigned Length;
   switch (kind) {
   case tok::charliteral:
   case tok::stringliteral:
   case tok::integerliteral:
   case tok::fpliteral:
   case tok::line_comment:
   case tok::block_comment:
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

void Token::dump() const
{
   print(std::cerr);
}

void Token::print(std::ostream &OS) const
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
   case tok::dollar_ident:
      OS << "$" << getIdentifier();
      return;
   case tok::line_comment:
   case tok::block_comment:
      OS << getText();
      break;
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

   return getIdentifierInfo()->getIdentifier() == str;
}

bool Token::isWhitespace() const
{
   switch (getKind()) {
   case tok::space: case tok::newline:
   case tok::line_comment: case tok::block_comment:
      return true;
   default:
      return false;
   }
}

std::string_view Token::getIdentifier() const
{
   return getIdentifierInfo()->getIdentifier();
}

uint64_t Token::getIntegerValue() const
{
   assert(kind == tok::integerliteral);

   auto txt = getText();
   unsigned offset = 0;

   if (txt[0] == '0') {
      if (txt.size() > 1) {
         if (txt[1] == 'x' || txt[1] == 'X') {
            offset = 2;
         }
         else if (txt[1] == 'b' || txt[1] == 'B') {
            offset = 2;
         }
         else {
            offset = 1;
         }
      }
   }

   std::string_view str(txt.data() + offset, txt.size() - offset);
   LiteralParser parser(str);

   return parser.parseInteger(64, true).APS;
}

string Token::toString() const
{
   std::ostringstream OS;
   print(OS);

   return OS.str();
}

string Token::rawRepr() const
{
   string s;
   if (kind == tok::space) {
      s += getText();
      return s;
   }

   switch (kind) {
   case tok::charliteral:
   case tok::stringliteral:
      s += std::string_view(reinterpret_cast<const char*>(Ptr) - 1, Data + 2);
      break;
   case tok::newline:
#ifdef _WIN32
      s += '\r';
#endif

      s += '\n';
      break;
#  define CDOT_PUNCTUATOR_TOKEN(Name, Spelling)                               \
   case tok::Name: s += (Spelling); break;
#  include "tblgen/Lex/Tokens.def"
   default:
      s += toString();
      break;
   }

   return s;
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