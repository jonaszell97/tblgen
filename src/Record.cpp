//
// Created by Jonas Zell on 01.02.18.
//

#include "tblgen/Record.h"
#include "tblgen/TableGen.h"
#include "tblgen/Value.h"

#include <llvm/Support/raw_ostream.h>

namespace tblgen {

Class *RecordKeeper::CreateClass(llvm::StringRef name, SourceLocation loc)
{
   auto C = new (TG) Class(*this, name, loc);
   Classes.try_emplace(name, C);

   return C;
}

void Class::dump()
{
   printTo(llvm::errs());
}

void Class::printTo(llvm::raw_ostream &out)
{
   out << "class " << name << " ";
   if (!parameters.empty()) {
      out << "<";

      size_t i = 0;
      for (auto &param : parameters) {
         if (i++ != 0) out << ", ";
         out << param.getName() << ": ";
         out << param.getType();

         if (auto V = param.getDefaultValue())
            out << " = " << V;
      }

      out << "> ";
   }

   if (!bases.empty()) {
      out << ": ";

      size_t i = 0;
      for (auto &base : bases) {
         if (i++ != 0) out << ", ";
         out << base.getBase()->getName();

         if (!base.getTemplateArgs().empty()) {
            out << "<";

            size_t j = 0;
            for (auto &TA : base.getTemplateArgs()) {
               if (j++ != 0) out << ", ";
               out << TA;
            }

            out << ">";
         }
      }
   }

   if (!fields.empty()) {
      out << " {\n";

      for (auto &F : fields) {
         out << "   " << F.getName() << ": " << F.getType();
         if (auto V = F.getDefaultValue())
            out << " = " << V;

         out << "\n";
      }

      out << "}";
   }
}

void Record::dump()
{
   printTo(llvm::errs());
}

void Record::dumpAllValues()
{
   auto &out = llvm::errs();
   out << "def " << name << " {\n";

   for (auto &F : fieldValues) {
      out << "   " << F.getKey() << " = " << F.getValue() << "\n";
   }

   out << "}";
}

void Record::printTo(llvm::raw_ostream &out)
{
   out << "def " << name << " ";

   if (!bases.empty()) {
      out << ": ";

      size_t i = 0;
      for (auto &base : bases) {
         if (i++ != 0) out << ", ";
         out << base.getBase()->getName();

         if (!base.getTemplateArgs().empty()) {
            out << "<";

            size_t j = 0;
            for (auto &TA : base.getTemplateArgs()) {
               if (j++ != 0) out << ", ";
               out << TA;
            }

            out << ">";
         }
      }
   }

   if (!ownFields.empty()) {
      out << " {\n";

      for (auto &F : ownFields) {
         out << "   " << F.getName() << " = " << F.getDefaultValue() << "\n";
      }

      out << "}";
   }
}

void Enum::printTo(llvm::raw_ostream &out)
{
   out << "enum " << name << " {\n";

   int i = 0;
   for (auto &Case : casesByName)
   {
      if (i++ != 0) out << ",\n";
      out << Case.getKey() << " = " << Case.getValue()->caseValue;
   }

   out << "\n}";
}

void Enum::addCase(llvm::StringRef caseName,
                   llvm::Optional<uint64_t> caseVal)
{
   assert(casesByName.count(caseName) == 0 && "duplicate case name");

   uint64_t val;
   if (caseVal.hasValue())
   {
      val = caseVal.getValue();
      assert(casesByValue.count(val) == 0 && "duplicate case value");
   }
   else
   {
      val = casesByName.size();
      while (casesByValue.count(val) != 0)
      {
         ++val;
      }
   }

   auto *c = new(RK.getAllocator()) EnumCase { caseName, val };
   casesByName.try_emplace(caseName, c);
   casesByValue.try_emplace(val, c);
}

llvm::Optional<uint64_t> Enum::getCaseValue(llvm::StringRef caseName) const
{
   auto it = casesByName.find(caseName);
   if (it == casesByName.end())
      return llvm::None;

   return it->getValue()->caseValue;
}

llvm::Optional<llvm::StringRef> Enum::getCaseName(uint64_t caseVal) const
{
   auto it = casesByValue.find(caseVal);
   if (it == casesByValue.end())
      return llvm::None;

   return it->getSecond()->caseName;
}

EnumCase *Enum::getCase(llvm::StringRef caseName) const
{
   auto it = casesByName.find(caseName);
   if (it == casesByName.end())
      return nullptr;

   return it->getValue();
}

EnumCase *Enum::getCase(uint64_t caseVal) const
{
   auto it = casesByValue.find(caseVal);
   if (it == casesByValue.end())
      return nullptr;

   return it->getSecond();
}

Record* RecordKeeper::CreateRecord(llvm::StringRef name, SourceLocation loc)
{
   auto R = new (TG) Record(*this, name, loc);
   Records.insert(std::make_pair(name, R));

   return R;
}

Record* RecordKeeper::CreateAnonymousRecord(tblgen::SourceLocation loc)
{
   return new(TG) Record(*this, loc);
}

Enum * RecordKeeper::CreateEnum(llvm::StringRef name,
                                tblgen::SourceLocation loc)
{
   auto E = new (TG) Enum(*this, name, loc);
   Enums.insert(std::make_pair(name, E));

   return E;
}

void RecordKeeper::dump()
{
   printTo(llvm::errs());
}

void RecordKeeper::printTo(llvm::raw_ostream &out)
{
   for (auto &C : Classes) {
      C.getValue()->printTo(out);
      out << "\n\n";
   }

   for (auto &R : Records) {
      R.getValue()->printTo(out);
      out << "\n\n";
   }

   for (auto &NS : Namespaces) {
      out << "namespace " << NS.getValue().getNamespaceName() << " {\n\n";

      NS.getValue().printTo(out);

      out << "\n}";
   }
}

void RecordKeeper::addValue(llvm::StringRef name,
                            Value *V,
                            SourceLocation loc) {
   Values[name] = ValueDecl(V, loc);
}

llvm::BumpPtrAllocator& RecordKeeper::getAllocator() const
{
   return TG.getAllocator();
}

} // namespace tblgen