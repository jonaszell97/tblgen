//
// Created by Jonas Zell on 30.01.18.
//

#include "tblgen/Basic/IdentifierInfo.h"

using namespace tblgen::lex;

namespace tblgen {

void IdentifierTable::addKeywords()
{
   if (KeywordsAdded)
      return;

#  define TBLGEN_KEYWORD_TOKEN(Name, Pattern)  \
   addKeyword(tok::Name, Pattern);
#  define TBLGEN_POUND_KEYWORD(Name)           \
   addKeyword(tok::pound_##Name, "#" #Name);
#  include "tblgen/Lex/Tokens.def"

   KeywordsAdded = true;
}

void IdentifierTable::addTblGenKeywords()
{
   addKeyword(tok::kw_class,   "class");
   addKeyword(tok::kw_let,     "let");
   addKeyword(tok::kw_def,     "def");
   addKeyword(tok::kw_enum,     "enum");
   addKeyword(tok::kw_true,    "true");
   addKeyword(tok::kw_false,   "false");
   addKeyword(tok::kw_in,        "in");
   addKeyword(tok::kw_namespace, "namespace");
   addKeyword(tok::tblgen_foreach, "foreach");
   addKeyword(tok::tblgen_print,   "print");
   addKeyword(tok::tblgen_if,      "if");

#  define TBLGEN_POUND_KEYWORD(Name)           \
   addKeyword(tok::pound_##Name, "#" #Name);
#  include "tblgen/Lex/Tokens.def"
}

void IdentifierTable::addKeyword(tblgen::lex::tok::TokenType kind,
                                 std::string_view kw) {
   (void)get(kw, kind);
}

} // namespace tblgen