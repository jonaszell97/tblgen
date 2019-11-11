
#ifndef TBLGEN_TOKENKINDS_H
#define TBLGEN_TOKENKINDS_H

namespace tblgen {
namespace lex {
namespace tok {

enum TokenType: unsigned short {
   __initial = 0,

#  define TBLGEN_TOKEN(Name, Spelling) Name,
#  define TBLGEN_TABLEGEN_KW_TOKEN(Name, Spelling) Name,
#  include "tblgen/Lex/Tokens.def"
};

} // namespace tok
} // namespace lex
} // namespace tblgen

#endif //TBLGEN_TOKENKINDS_H
