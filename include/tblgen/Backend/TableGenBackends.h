//
// Created by Jonas Zell on 04.02.18.
//

#ifndef TBLGEN_TABLEGENBACKENDS_H
#define TBLGEN_TABLEGENBACKENDS_H

namespace llvm {
   class raw_ostream;
} // namespace llvm

namespace tblgen {

class RecordKeeper;

void PrintRecords(llvm::raw_ostream &str, RecordKeeper const& RK);
void EmitClassHierarchy(llvm::raw_ostream &str, RecordKeeper const& RK);

} // namespace tblgen

#endif //TBLGEN_TABLEGENBACKENDS_H
