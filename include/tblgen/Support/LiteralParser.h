
#ifndef TBLGEN_LITERALPARSER_H
#define TBLGEN_LITERALPARSER_H

#include <cassert>
#include <string>

namespace tblgen {

class LiteralParser {
public:
   explicit LiteralParser(std::string_view Str)
      : Str(Str)
   {
      assert(!Str.empty() && "empty literal");
   }

   struct IntResult {
      uint64_t APS;
      bool wasTruncated;
   };

   IntResult parseInteger(unsigned bitwidth = 64, bool isSigned = true);

   struct FPResult {
      double APF;
   };

   FPResult parseFloating();

   struct CharResult {
      uint32_t Char;
      bool Malformed;
   };

   CharResult parseCharacter();

   struct StringResult {
      std::string Str;
      bool Malformed;
   };

   StringResult parseString();

private:
   std::string_view Str;

   uint8_t getIntegerRadix();
};

} // namespace tblgen

#endif //TBLGEN_LITERALPARSER_H
