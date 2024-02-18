
#include "tblgen/Message/Diagnostics.h"
#include "tblgen/TableGen.h"
#include "tblgen/Record.h"
#include "tblgen/Value.h"
#include "tblgen/Support/Casting.h"

using namespace tblgen::support;

namespace tblgen {

TableGen::TableGen(support::ArenaAllocator &Allocator,
                   fs::FileManager &fileMgr,
                   DiagnosticsEngine &Diags)
   : Allocator(Allocator), fileMgr(fileMgr), Diags(Diags),
     GlobalRK(nullptr),
     Idents(Allocator, 1024),
     Int1Ty(1, false),
     Int8Ty(8, false),   UInt8Ty(8, true),
     Int16Ty(16, false), UInt16Ty(16, true),
     Int32Ty(32, false), UInt32Ty(32, true),
     Int64Ty(64, false), UInt64Ty(64, true),
     Undef(&UndefTy)
{
   RecordKeeper *storage = this->Allocate<RecordKeeper>();
   this->GlobalRK = new (storage) RecordKeeper(*this);
}

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

      std::string key(DA->getKey());
      return cast<DictLiteral>(dict)->getValue(key);
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

static Value *getOverride(const TableGen &TG, Record &R, std::string_view FieldName)
{
   for (auto &Base : R.getBases()) {
      if (auto *OV = Base.getBase()->getOverride(FieldName)) {
         if (OV->isAppend())
         {
            auto baseValue = Base.getBase()->getOwnField(FieldName);
            auto *defaultVal = baseValue->getDefaultValue();

            assert(defaultVal != nullptr && "no default value for overriden field");

            if (auto *list = cast<ListLiteral>(defaultVal))
            {
               auto *listToAppend = cast<ListLiteral>(OV->getDefaultValue());
               std::vector<Value*> vec(list->getValues());
               vec.insert(vec.end(), listToAppend->getValues().begin(),
                          listToAppend->getValues().end());

               return new(TG) ListLiteral(list->getType(), move(vec));
            }
            else
            {
               auto *dict = cast<DictLiteral>(defaultVal);
               auto *dictToAppend = cast<DictLiteral>(OV->getDefaultValue());

               auto map = dict->getValues();
               map.insert(dictToAppend->getValues().begin(),
                          dictToAppend->getValues().end());

               return new(TG) DictLiteral(dict->getType(), move(map));
            }
         }

         return OV->getDefaultValue();
      }
   }

   return nullptr;
}

static RecordField const*
implementBaseForRecord(const TableGen &TG, Class::BaseClass const& Base,
                       Record &R,
                       const std::vector<Value *> &BaseTemplateArgs) {
   for (auto &Field : Base.getBase()->getFields()) {
      std::string fieldName(Field.getName());
      if (auto val = R.getOwnField(Field.getName())) {
         R.setFieldValue(fieldName, val->getDefaultValue());
      }
      else if (auto Override = getOverride(TG, R, Field.getName())) {
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

      if (auto missing = implementBaseForRecord(TG, NextBase, R,
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
      if (auto missing = implementBaseForRecord(*this, Base, R,
                                                Base.getTemplateArgs())) {
         return {
            RFS_MissingFieldValue, std::string(missing->getName()),
            missing->getDeclLoc()
         };
      }
   }

   auto name = R.hasField("name") ? "__name" : "name";
   R.addOwnField(SourceLocation(), name, getStringTy(),
      new (*this) StringLiteral(getStringTy(), std::string(R.getName())));

   return { RFS_Success };
}

} // namespace tblgen