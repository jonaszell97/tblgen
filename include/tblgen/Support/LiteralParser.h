//
// Created by Jonas Zell on 13.02.18.
//

#ifndef TBLGEN_LITERALPARSER_H
#define TBLGEN_LITERALPARSER_H

#include <llvm/ADT/StringRef.h>
#include <llvm/ADT/APFloat.h>
#include <llvm/ADT/APSInt.h>

namespace tblgen {

class LiteralParser {
public:
   LiteralParser(std::string_view Str)
      : Str(Str)
   {
      assert(!Str.empty() && "empty literal");
   }

   struct IntResult {
      llvm::APSInt APS;
      bool wasTruncated;
   };

   IntResult parseInteger(unsigned bitwidth = 64, bool isSigned = true);

   struct FPResult {
      llvm::APFloat APF;
      llvm::APFloat::opStatus status;
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
