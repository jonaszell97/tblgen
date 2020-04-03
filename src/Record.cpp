
#include "tblgen/Record.h"
#include "tblgen/TableGen.h"
#include "tblgen/Value.h"

#include <iostream>

using std::string;

namespace tblgen {

Class *RecordKeeper::CreateClass(const std::string &name, SourceLocation loc)
{
   auto C = new (TG) Class(*this, name, loc);
   Classes.emplace(name, C);

   return C;
}

void Class::dump()
{
   printTo(std::cerr);
}

void Class::printTo(std::ostream &out)
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
   printTo(std::cerr);
}

void Record::dumpAllValues()
{
   auto &out = std::cerr;
   out << "def " << name << " {\n";

   for (auto &F : fieldValues) {
      out << "   " << F.first << " = " << F.second << "\n";
   }

   out << "}";
}

void Record::printTo(std::ostream &out)
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

void Enum::printTo(std::ostream &out)
{
   out << "enum " << name << " {\n";

   int i = 0;
   for (auto &Case : casesByName)
   {
      if (i++ != 0) out << ",\n";
      out << Case.first << " = " << Case.second->caseValue;
   }

   out << "\n}";
}

void Enum::addCase(std::string_view caseName,
                   support::Optional<uint64_t> caseVal)
{
   string name(caseName);
   assert(casesByName.count(name) == 0 && "duplicate case name");

   uint64_t val;
   if (caseVal.hasValue())
   {
      val = caseVal;
      assert(casesByValue.count(val) == 0 && "duplicate case value");
   }
   else if (!casesByValue.empty())
   {
      val = std::max_element(casesByValue.begin(), casesByValue.end(),
         [](std::unordered_map<uint64_t, EnumCase*>::value_type const &v1,
            std::unordered_map<uint64_t, EnumCase*>::value_type const &v2) {
         return v1.first > v2.first;
      })->first;

      while (casesByValue.count(val) != 0)
      {
         ++val;
      }
   }
   else
   {
      val = 0;
   }

   auto *c = new(RK.getAllocator()) EnumCase { move(name), val };
   casesByName.emplace(caseName, c);
   casesByValue.emplace(val, c);
}

support::Optional<uint64_t> Enum::getCaseValue(const string &caseName) const
{
   auto it = casesByName.find(caseName);
   if (it == casesByName.end())
      return support::None;

   return it->second->caseValue;
}

support::Optional<std::string_view> Enum::getCaseName(uint64_t caseVal) const
{
   auto it = casesByValue.find(caseVal);
   if (it == casesByValue.end())
      return support::None;

   return std::string_view(it->second->caseName);
}

EnumCase *Enum::getCase(const string &caseName) const
{
   auto it = casesByName.find(caseName);
   if (it == casesByName.end())
      return nullptr;

   return it->second;
}

EnumCase *Enum::getCase(uint64_t caseVal) const
{
   auto it = casesByValue.find(caseVal);
   if (it == casesByValue.end())
      return nullptr;

   return it->second;
}

Record* RecordKeeper::CreateRecord(const std::string &name, SourceLocation loc)
{
   auto R = new (TG) Record(*this, name, loc);
   Records.push_back(R);
   RecordsMap.emplace(name, R);

   return R;
}

Record* RecordKeeper::CreateAnonymousRecord(tblgen::SourceLocation loc)
{
   return new(TG) Record(*this, loc);
}

Enum * RecordKeeper::CreateEnum(const std::string &name,
                                tblgen::SourceLocation loc)
{
   auto E = new (TG) Enum(*this, name, loc);
   Enums.insert(std::make_pair(name, E));

   return E;
}

void RecordKeeper::dump()
{
   printTo(std::cerr);
}

void RecordKeeper::printTo(std::ostream &out)
{
   for (auto &C : Classes) {
      C.second->printTo(out);
      out << "\n\n";
   }

   for (auto &R : Records) {
      R->printTo(out);
      out << "\n\n";
   }

   for (auto &NS : Namespaces) {
      out << "namespace " << NS.second->getNamespaceName() << " {\n\n";

      NS.second->printTo(out);

      out << "\n}";
   }
}

void RecordKeeper::addValue(const std::string &name,
                            Value *V,
                            SourceLocation loc) {
   Values[name] = ValueDecl(V, loc);
}

RecordKeeper *RecordKeeper::addNamespace(const std::string &name,
                                         SourceLocation loc) {
   auto *RK = new(TG.getAllocator()) RecordKeeper(TG, name, loc, this);
   Namespaces.emplace(name, RK);

   return RK;
}

support::ArenaAllocator& RecordKeeper::getAllocator() const
{
   return TG.getAllocator();
}

} // namespace tblgen