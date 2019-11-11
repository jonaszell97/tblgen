
#include "tblgen/Message/Diagnostics.h"
#include "tblgen/TableGen.h"
#include "tblgen/Record.h"
#include "tblgen/Value.h"
#include "tblgen/Support/Casting.h"

using namespace tblgen::support;

namespace tblgen {

static Value *resolveValue(Value *V,
                           Class::BaseClass const &PreviousBase,
                           const std::vector<Value *> &ConcreteTemplateArgs,
                           SourceLocation errorLoc = {}) {
   if (auto Id = dyn_cast<IdentifierVal>(V)) {
      size_t i = 0;
      Value *Val = nullptr;
      for (auto &P : PreviousBase.getBase()->getParameters()) {
         if (P.getName() == Id->getVal()) {
            if (i < ConcreteTemplateArgs.size()) {
               Val = ConcreteTemplateArgs[i];
            }
            else {
               Val = P.getDefaultValue();
            }

            break;
         }

         ++i;
      }

      assert(Val);
      return Val;
   }
   else if (auto DA = dyn_cast<DictAccessExpr>(V)) {
      auto dict = resolveValue(DA->getDict(), PreviousBase,
                               ConcreteTemplateArgs, errorLoc);
//      if (!isa<DictLiteral>(dict))
//         diag::err(diag::err_generic_error)
//            << "value is not a dictionary"
//            << errorLoc << diag::term;

      std::string key(DA->getKey());
      auto val = cast<DictLiteral>(dict)->getValue(key);
//      if (!val)
//         diag::err(diag::err_generic_error)
//            << "key does not exist in dictionary"
//            << errorLoc << diag::term;

      return val;
   }
   else {
      return V;
   }
}

static void resolveValues(Class::BaseClass const &Base,
                          Class::BaseClass const &PreviousBase,
                          const std::vector<Value *> &ConcreteTemplateArgs,
                          std::vector<Value *> &DstValues) {
   for (auto &V : Base.getTemplateArgs()) {
      DstValues.push_back(resolveValue(V, PreviousBase, ConcreteTemplateArgs));
   }
}

static Value *getOverride(Record &R, std::string_view FieldName)
{
   for (auto &Base : R.getBases()) {
      if (auto *OV = Base.getBase()->getOverride(FieldName)) {
         return OV->getDefaultValue();
      }
   }

   return nullptr;
}

static RecordField const*
implementBaseForRecord(Class::BaseClass const& Base,
                       Record &R,
                       const std::vector<Value *> &BaseTemplateArgs) {
   for (auto &Field : Base.getBase()->getFields()) {
      std::string fieldName(Field.getName());
      if (auto val = R.getOwnField(Field.getName())) {
         R.setFieldValue(fieldName, val->getDefaultValue());
      }
      else if (auto Override = getOverride(R, Field.getName())) {
         R.setFieldValue(fieldName, resolveValue(Override, Base,
                                                       BaseTemplateArgs,
                                                       Field.getDeclLoc()));
      }
      else if (auto def = Field.getDefaultValue()) {
         R.setFieldValue(fieldName, resolveValue(def, Base,
                                                       BaseTemplateArgs,
                                                       Field.getDeclLoc()));
      }
      else if (Field.hasAssociatedTemplateParm()) {
         size_t idx = Field.getAssociatedTemplateParm();

         assert(idx < Base.getBase()->getParameters().size()
                && "invalid template parameter index");

         if (idx < Base.getTemplateArgs().size()) {
            R.setFieldValue(fieldName,
                            resolveValue(Base.getTemplateArgs()[idx], Base,
                                         BaseTemplateArgs, Field.getDeclLoc()));
         }
         else {
            auto P = Base.getBase()->getParameters()[idx];
            assert (P.getDefaultValue() && "template parm not supplied!");
            R.setFieldValue(fieldName,
                            resolveValue(P.getDefaultValue(), Base,
                                         BaseTemplateArgs,
                                         Field.getDeclLoc()));
         }
      }
      else {
         return &Field;
      }
   }

   // propagate resolved template arguments to the next base
   std::vector<Value*> NextBaseTemplateArgs;
   for (auto &NextBase : Base.getBase()->getBases()) {
      resolveValues(NextBase, Base, BaseTemplateArgs, NextBaseTemplateArgs);

      if (auto missing = implementBaseForRecord(NextBase, R,
                                                NextBaseTemplateArgs)) {
         return missing;
      }

      NextBaseTemplateArgs.clear();
   }

   return nullptr;
}

TableGen::FinalizeResult TableGen::finalizeRecord(Record &R)
{
   for (auto &Base : R.getBases()) {
      if (auto missing = implementBaseForRecord(Base, R,
                                                Base.getTemplateArgs())) {
         return {
            RFS_MissingFieldValue, std::string(missing->getName()),
            missing->getDeclLoc()
         };
      }
   }

   return { RFS_Success };
}

} // namespace tblgen