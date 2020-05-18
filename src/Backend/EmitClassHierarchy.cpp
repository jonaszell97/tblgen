
#include "tblgen/Backend/TableGenBackends.h"
#include "tblgen/Basic/DependencyGraph.h"
#include "tblgen/Message/Diagnostics.h"
#include "tblgen/Record.h"
#include "tblgen/Support/Casting.h"
#include "tblgen/Value.h"

#include <iostream>

using namespace tblgen::diag;
using namespace tblgen::support;

namespace tblgen {

static void buildMacroName(Record *R, std::string &macro,
                           std::string_view prefix, std::string_view suffix,
                           unsigned nameCutoff)
{
   macro.clear();

   unsigned offset = 1;
   if (!prefix.empty()) {
      macro += prefix;
      macro += "_";
      offset += prefix.size() + 1;
   }

   bool lastWasUnderscore = true;
   if (R) {
      macro += R->getName().substr(0, R->getName().size() - nameCutoff);
      lastWasUnderscore = false;
   }

   for (auto it = macro.begin() + offset; it < macro.end();) {
      if (::isupper(*it) && !lastWasUnderscore) {
         lastWasUnderscore = true;
         it = macro.insert(it, '_');
         it += 2;
      }
      else {
         lastWasUnderscore = false;
         ++it;
      }
   }

   if (!suffix.empty()) {
      if (!lastWasUnderscore)
         macro += "_";

      macro += suffix;
   }

   std::transform(macro.begin(), macro.end(), macro.begin(), ::toupper);
}

static bool isAbstract(Record *R)
{
   auto F = R->getFieldValue("Abstract");
   if (!F || !isa<IntegerLiteral>(F))
      return false;

   return cast<IntegerLiteral>(F)->getVal() != 0;
}

void EmitClassHierarchy(std::ostream &out, RecordKeeper const &RK)
{
   Class *BaseClass = nullptr;
   Class *DerivedClass = nullptr;

   auto options = RK.lookupRecord("HierarchyOptions");
   if (!options) {
      std::cerr << "no HierarchyOptions def provided\n";

      return;
   }

   auto BaseVal = options->getFieldValue("BaseClass");
   if (!BaseVal || !isa<StringLiteral>(BaseVal)) {
      std::cerr
          << "HierarchyOptions must have a field 'BaseClass' of type string\n";

      return;
   }

   BaseClass = RK.lookupClass(cast<StringLiteral>(BaseVal)->getVal());
   if (!BaseClass) {
      std::cerr << "class " << cast<StringLiteral>(BaseVal)->getVal()
                << " not found\n";

      return;
   }

   auto DerivedVal = options->getFieldValue("DerivedClass");
   if (!DerivedVal || !isa<StringLiteral>(DerivedVal)) {
      std::cerr << "HierarchyOptions must have a field 'DerivedClass' of type "
                   "string\n";

      return;
   }

   DerivedClass = RK.lookupClass(cast<StringLiteral>(DerivedVal)->getVal());
   if (!DerivedClass) {
      std::cerr << "class " << cast<StringLiteral>(DerivedVal)->getVal()
                << " not found\n";

      return;
   }

   std::string_view macroPrefix = "";
   std::string_view macroSuffix = "DECL";
   unsigned nameCutoff = 0;

   if (auto prefix = options->getFieldValue("MacroPrefix")) {
      if (auto S = dyn_cast<StringLiteral>(prefix))
         macroPrefix = S->getVal();
   }

   if (auto suffix = options->getFieldValue("MacroSuffix")) {
      if (auto S = dyn_cast<StringLiteral>(suffix))
         macroSuffix = S->getVal();
   }

   if (auto cutoff = options->getFieldValue("NameCutoff")) {
      if (auto I = dyn_cast<IntegerLiteral>(cutoff))
         nameCutoff = unsigned(I->getVal());
   }

   struct Decl {
      Decl(Record *R, Record *Base, std::vector<Decl *> &&SubDecls,
           std::string &&macroName)
          : R(R), Base(Base), SubDecls(move(SubDecls)),
            macroName(move(macroName))
      {
      }

      Record *R;
      Record *Base;
      std::vector<Decl *> SubDecls;
      std::string macroName;
   };

   std::unordered_map<Record *, Decl> DeclMap;
   std::string macro;

   buildMacroName(nullptr, macro, macroPrefix, macroSuffix, nameCutoff);

   DeclMap.emplace(nullptr,
                   Decl(nullptr, nullptr, std::vector<Decl *>(), std::string(macro)));

   std::vector<Record *> DeclVec;
   RK.getAllDefinitionsOf(BaseClass, DeclVec);

   DependencyGraph<Record *> DG;
   for (auto &D : DeclVec) {
      auto &node = DG.getOrAddVertex(D);
      auto Base = D->getFieldValue("Base");

      if (auto RecVal = dyn_cast_or_null<RecordVal>(Base)) {
         auto &baseNode = DG.getOrAddVertex(RecVal->getRecord());
         baseNode.addOutgoing(&node);
      }
   }

   auto order = DG.constructOrderedList();
   assert(order.second && "record used before declaration");

   for (auto &D : order.first) {
      auto Base = D->getFieldValue("Base");
      Record *BaseRec = nullptr;

      if (Base && isa<RecordVal>(Base)) {
         BaseRec = cast<RecordVal>(Base)->getRecord();
      }

      auto it = DeclMap.find(BaseRec);
      assert(it != DeclMap.end() && "invalid decl order");

      buildMacroName(D, macro, macroPrefix, macroSuffix, nameCutoff);

      auto E = DeclMap.emplace(D, Decl(D, BaseRec, {}, std::string(macro)));
      it->second.SubDecls.push_back(&E.first->second);
   }

   order.first.insert(order.first.begin(), nullptr);

   size_t i = 0;
   for (auto &ptr : order.first) {
      auto &D = *DeclMap.find(ptr);
      if (D.second.SubDecls.empty())
         continue;

      if (i++ != 0)
         out << "\n\n";
      out << "#ifdef " << D.second.macroName << "\n";

      for (auto &Sub : D.second.SubDecls) {
         if (Sub->SubDecls.empty())
            continue;

         out << "#  ifndef " << Sub->macroName << "\n";
         out << "#    define " << Sub->macroName << "(Name) "
             << D.second.macroName << "(Name)\n";
         out << "#  endif\n";
      }

      if (D.second.R && !isAbstract(D.second.R))
         out << "  " << D.second.macroName << "(" << D.second.R->getName()
             << ")\n";

      for (auto &Sub : D.second.SubDecls) {
         if (!Sub->SubDecls.empty())
            continue;
         out << "  " << D.second.macroName << "(" << Sub->R->getName() << ")\n";
      }

      out << "#endif";
   }

   out << "\n\n";
   for (auto &ptr : order.first) {
      auto &D = *DeclMap.find(ptr);
      if (D.second.SubDecls.empty())
         continue;

      out << "#undef " << D.second.macroName << "\n";
   }
}

} // namespace tblgen