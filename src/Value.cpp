
#include "tblgen/Value.h"

#include "tblgen/TableGen.h"
#include "tblgen/Record.h"
#include "tblgen/Type.h"
#include "tblgen/Support/Casting.h"
#include "tblgen/Support/Format.h"

#include <sstream>

using namespace tblgen::support;

namespace tblgen {

Value::~Value()
{

}

std::ostream &operator<<(std::ostream &str, Value const* V)
{
   if (auto I = dyn_cast<IntegerLiteral>(V)) {
      auto type = cast<IntType>(I->getType());
      auto bitwidth = type->getBitWidth();
      if (bitwidth == 1) {
         str << (I->getVal() != 0 ? "true" : "false");
      }
      else if (bitwidth == 8 && !type->isUnsigned()) {
         str << "'";
         support::unescape_char((char)I->getVal(), str);
         str << "'";
      }
      else if (type->isUnsigned()) {
         str << I->getVal();
      }
      else {
         str << (int64_t)I->getVal();
      }
   }
   else if (auto FP = dyn_cast<FPLiteral>(V)) {
      str << FP->getVal();
   }
   else if (auto S = dyn_cast<StringLiteral>(V)) {
      str << '"' << S->getVal() << '"';
   }
   else if (auto CB = dyn_cast<CodeBlock>(V)) {
      str << "{" << CB->getCode() << "}";
   }
   else if (auto Id = dyn_cast<IdentifierVal>(V)) {
      str << Id->getVal();
   }
   else if (auto L = dyn_cast<ListLiteral>(V)) {
      str << "[";

      size_t i = 0;
      for (auto &el : L->getValues()) {
         if (i++ != 0) str << ", ";
         str << el;
      }
      str << "]";
   }
   else if (auto Dict = dyn_cast<DictLiteral>(V)) {
      str << "[";

      size_t i = 0;
      for (auto &el : Dict->getValues()) {
         if (i++ != 0) str << ", ";
         str << '"' << el.first << "\": " << el.second;
      }
      str << "]";
   }
   else if (auto R = dyn_cast<RecordVal>(V)) {
      auto Rec = R->getRecord();
      if (Rec->isAnonymous()) {
         auto &Base = Rec->getBases().front();
         str << Base.getBase()->getName();

         if (!Base.getTemplateArgs().empty()) {
            str << "<";

            size_t i = 0;
            for (auto &TA : Base.getTemplateArgs()) {
               if (i++ != 0) str << ", ";
               str << TA;
            }

            str << ">";
         }
      }
      else {
         str << R->getRecord()->getName();
      }
   }
   else if (auto DA = dyn_cast<DictAccessExpr>(V)) {
      str << DA->getDict() << "[\"" << DA->getKey() << "\"]";
   }
   else if (auto EV = dyn_cast<EnumVal>(V)) {
      str << EV->getType() << "." << EV->getCase()->caseName;
   }
   else {
      unreachable("unhandled value kind");
   }

   return str;
}

} // namespace tblgen