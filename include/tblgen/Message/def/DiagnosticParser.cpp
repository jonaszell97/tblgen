////
//// Created by Jonas Zell on 04.10.17.
////
//
//#include <cassert>
//#include <sstream>
//
//#include "tblgen/DiagnosticParser.h"
//#include "tblgen/../../lex/Lexer.h"
//#include "tblgen/../../Basic/IdentifierInfo.h"
//
//using namespace tblgen::lex;
//
//namespace tblgen {
//namespace diag {
//
//void DiagnosticParser::doParse()
//{
//   auto base = string(__FILE__);
//   base = base.substr(0, base.rfind('/'));
//
//   std::ifstream file(base + "/raw/errors.def");
//   parseFile(file, base);
//
//   file.close();
//}
//
//void DiagnosticParser::parseFile(std::ifstream &file, string& base)
//{
//   std::stringstream errorEnumOut;
//   std::stringstream errorMsgOut;
//
//   std::stringstream warnEnumOut;
//   std::stringstream warnMsgOut;
//
//   std::stringstream noteEnumOut;
//   std::stringstream noteMsgOut;
//
//   string line;
//   while (std::getline(file, line)) {
//      if (line.empty()) {
//         continue;
//      }
//
//      auto Buf = llvm::MemoryBuffer::getMemBuffer(line);
//
//      IdentifierTable II(16);
//      Lexer lex(II, Buf.get(), 0, '\0');
//      lex.lex();
//
//      if (lex.currentTok().is(tok::eof)) {
//         continue;
//      }
//
//      string name = lex.getCurrentIdentifier();
//      lex.advance();
//
//      assert(lex.currentTok().is(tok::smaller) && "expected <");
//      lex.advance();
//
//      string type = lex.getCurrentIdentifier();
//      lex.advance();
//
//      assert(lex.currentTok().is(tok::greater) && "expected >");
//      lex.advance();
//
//      assert(lex.currentTok().is(tok::stringliteral) && "expected string "
//         "literal");
//
//      string str = lex.currentTok().getText();
//
//      auto enumVal = type + '_' + name;
//
//      if (type == "err") {
//         errorEnumOut << enumVal << ",";
//         errorMsgOut << "{ " << enumVal << ", \"\\\"" << str << "\\\"\"},";
//      }
//      else if (type == "warn") {
//         warnEnumOut << enumVal << ",";
//         warnMsgOut << "{ " << enumVal << ", \"\\\"" << str << "\\\"\"},";
//      }
//      else if (type == "note") {
//         noteEnumOut << enumVal << ",";
//         noteMsgOut << "{ " << enumVal << ", \"\\\"" << str << "\\\"\"},";
//      }
//      else {
//         unreachable("unknwon diagnostic type");
//      }
//   }
//
//   std::ofstream errorEnumOutS(base + "/parsed/errors_enum.def");
//   std::ofstream errorMsgOutS(base + "/parsed/errors_msg.def");
//
//   errorEnumOutS << errorEnumOut.str();
//   errorMsgOutS << errorMsgOut.str();
//
//   errorEnumOutS.flush();
//   errorMsgOutS.flush();
//
//   errorEnumOutS.close();
//   errorMsgOutS.close();
//
//   std::ofstream warnEnumOutS(base + "/parsed/warn_enum.def");
//   std::ofstream warnMsgOutS(base + "/parsed/warn_msg.def");
//
//   warnEnumOutS << warnEnumOut.str();
//   warnMsgOutS << warnMsgOut.str();
//
//   warnEnumOutS.flush();
//   warnMsgOutS.flush();
//
//   warnEnumOutS.close();
//   warnMsgOutS.close();
//
//   std::ofstream noteEnumOutS(base + "/parsed/note_enum.def");
//   std::ofstream noteMsgOutS(base + "/parsed/note_msg.def");
//
//   noteEnumOutS << noteEnumOut.str();
//   noteMsgOutS << noteMsgOut.str();
//
//   noteEnumOutS.flush();
//   noteMsgOutS.flush();
//
//   noteEnumOutS.close();
//   noteMsgOutS.close();
//}
//
//}
//}