
#include "tblgen/Record.h"
#include "tblgen/Value.h"

#include <iostream>

namespace tblgen {

static void printBases(std::ostream &out, Class &C, size_t &i)
{
   for (auto &B : C.getBases()) {
      if (i++ != 0) out << ", ";
      out << B.getBase()->getName();
      printBases(out, *B.getBase(), i);
   }
}

static void printBases(std::ostream &out, Record &R, size_t &i)
{
   for (auto &B : R.getBases()) {
      if (i++ != 0) out << ", ";
      out << B.getBase()->getName();
      printBases(out, *B.getBase(), i);
   }
}

static void printRecord(std::ostream &out, Record &R)
{
   out << "def " << R.getName() << " {";
   if (!R.getBases().empty()) {
      out << " // ";
      size_t i = 0;
      printBases(out, R, i);
   }

   out << "\n";

   for (auto &valPair : R.getFieldValues()) {
      out << "   " << valPair.first << " = " << valPair.second << "\n";
   }

   out << "}";
}

extern "C" void EmitPrettyPrint(std::ostream &str, RecordKeeper const& RK)
{
   size_t i = 0;
   for (auto &R : RK.getAllRecords()) {
      if (i++ != 0) str << "\n\n";
      printRecord(str, *R);
   }

   for (auto &NS : RK.getAllNamespaces()) {
      str << "\n\nnamespace " << NS.second->getNamespaceName() << " {\n\n";

      EmitPrettyPrint(str, *NS.second);

      str << "\n\n} // end namespace " << NS.second->getNamespaceName();
   }
}

} // namespace tblgen