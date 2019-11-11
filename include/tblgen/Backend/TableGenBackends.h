
#ifndef TBLGEN_TABLEGENBACKENDS_H
#define TBLGEN_TABLEGENBACKENDS_H

#include <iosfwd>

namespace tblgen {

class RecordKeeper;

void PrintRecords(std::ostream &str, RecordKeeper const& RK);
void EmitClassHierarchy(std::ostream &str, RecordKeeper const& RK);

} // namespace tblgen

#endif //TBLGEN_TABLEGENBACKENDS_H
