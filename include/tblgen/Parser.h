
#ifndef TABLEGEN_PARSER_H
#define TABLEGEN_PARSER_H

#include "tblgen/Basic/IdentifierInfo.h"
#include "tblgen/Lex/Lexer.h"
#include "tblgen/Message/DiagnosticsEngine.h"
#include "tblgen/TableGen.h"

#include <array>
#include <sstream>

namespace tblgen {

class RecordKeeper;
class Record;
class Class;
class Enum;
class TableGen;
class Value;
class Type;

class Parser {
public:
   Parser(TableGen &TG,
          const std::string &Buf,
          unsigned sourceId,
          unsigned baseOffset);

   Parser(TableGen &TG,
          const std::vector<lex::Token> &Toks,
          unsigned sourceId,
          unsigned baseOffset);

   ~Parser();

   bool parse();

protected:
   TableGen &TG;
   lex::Lexer lex;
   Class *currentClass = nullptr;
   RecordKeeper *RK;

   lex::Lexer::LookaheadRAII *LR = nullptr;
   std::unordered_map<std::string, Value*> ForEachVals;

   [[noreturn]]
   void abortBP();

   void parseNextDecl();
   void parseRecordLevelDecl(Record *R);
   void parseClassLevelDecl(Class *C);

   void parseClass();
   void parseTemplateParams(Class *C,
                            std::vector<size_t> &fieldParameters);
   void parseBases(Class *C);
   void parseFieldDecl(Class *C);
   void parseOverrideDecl(Class *C, bool isAppend);

   void parseRecord();
   void parseBases(Record *R);
   void parseFieldDef(Record *R);

   void finalizeRecord(Record &R);
   void validateTemplateArgs(Class &C,
                             std::vector<SourceLocation> &locs,
                             std::vector<Value*> &givenParams);

   void parseEnum();
   void parseEnumCase(Enum *E);

   void parseValue();

   void parseInclude();
   void parseIf(Class *C = nullptr, Record *R = nullptr);
   void parseForEach(Class *C = nullptr, Record *R = nullptr);
   void parsePrint();
   void parseNamespace();

   std::string tryParseIdentifier();

   Type *parseType();
   Value *parseExpr(Type *contextualTy = nullptr);
   Value *parseFunction(Type *contextualTy = nullptr);

   void parseTemplateArgs(std::vector<Value*> &args,
                          std::vector<SourceLocation> &locs,
                          Class *forClass);

   struct NamespaceScope {
      NamespaceScope(Parser &P, RecordKeeper *NewRK)
         : P(P), RK(P.RK)
      {
         P.RK = NewRK;
      }

      ~NamespaceScope()
      {
         P.RK = RK;
      }

   private:
      Parser &P;
      RecordKeeper *RK;
   };

   struct ForEachScope {
      ForEachScope(Parser &P, std::string_view name, Value *V)
         : P(P), name(name)
      {
         P.ForEachVals.emplace(name, V);
      }

      ForEachScope(Parser &P, const std::string &name, Value *V)
         : P(P), name(name)
      {
         P.ForEachVals.emplace(name, V);
      }

      ~ForEachScope()
      {
         P.ForEachVals.erase(P.ForEachVals.find(name));
      }

   private:
      Parser &P;
      std::string name;
   };

   Value *getForEachVal(const std::string &name)
   {
      auto it = ForEachVals.find(name);
      if (it != ForEachVals.end())
         return it->second;

      return nullptr;
   }

   lex::Token const& currentTok() const
   {
      return lex.currentTok();
   }

   void advance(bool ignoreNewline = true, bool ignoreWhitespace = true)
   {
      if (LR) {
         return LR->advance(ignoreNewline, !ignoreWhitespace);
      }

      return lex.advance(ignoreNewline, !ignoreWhitespace);
   }

   lex::Token peek(bool ignoreNewline = true, bool ignoreWhitespace = true)
   {
      return lex.lookahead(ignoreNewline, !ignoreWhitespace);
   }

   template<class ...Args>
   void expect(Args ...kinds)
   {
      advance();
      if (!currentTok().oneOf(kinds...)) {
         TG.Diags.Diag(diag::err_generic_error)
            << "unexpected token " + currentTok().toString()
            << lex.getSourceLoc();

         exit(1);
      }
   }
};

class TemplateParser: public Parser {
public:
   TemplateParser(TableGen &TG,
                  const std::string &Buf,
                  unsigned sourceId,
                  unsigned baseOffset);

   TemplateParser(TableGen &TG,
                  const std::vector<lex::Token> &Toks,
                  unsigned sourceId,
                  unsigned baseOffset);

   bool parseTemplate();
   std::string getResult();

private:
   struct Macro
   {
      std::vector<std::string> params;
      std::vector<lex::Token> tokens;
   };

   std::ostringstream OS;
   std::ostringstream *ActiveOS;
   std::vector<lex::Token> currentTokens;
   std::unordered_map<std::string, Macro> macros;

   bool commandFollows();

   void appendTokens(bool beforeCommand = false);

   void parseUntilEnd(bool *isElse = nullptr);
   void skipUntilEnd(std::vector<lex::Token> *tokens = nullptr);

   void advanceNoSkip()
   {
      advance(false, false);
   }

   lex::Token peekNoSkip()
   {
      return peek(false, false);
   }

   void handleTemplateExpr(bool *isEnd = nullptr, bool *isElse = nullptr);

   Value *handleCommand(bool paste, bool &explicitStr,
                        bool *isEnd = nullptr, bool *isElse = nullptr);

   Value *handleIf(bool paste);
   Value *handleForeach(bool paste);

   Value *handleDefine(bool paste);
   Value *handleInvoke(bool paste);
};

} // namespace tblgen


#endif //TABLEGEN_PARSER_H
