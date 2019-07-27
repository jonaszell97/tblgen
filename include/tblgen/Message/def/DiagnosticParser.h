//
// Created by Jonas Zell on 04.10.17.
//

#ifndef TBLGEN_DIAGNOSTICPARSER_H
#define TBLGEN_DIAGNOSTICPARSER_H

#include <fstream>

using std::string;

namespace tblgen {
namespace diag {

class DiagnosticParser {
public:
   DiagnosticParser() = default;
   void doParse();

protected:
   void parseFile(std::ifstream& file, string& base);
};

}
}

#endif //TBLGEN_DIAGNOSTICPARSER_H
