//
// Created by Jonas Zell on 01.02.18.
//

#ifndef TABLEGEN_PARSER_H
#define TABLEGEN_PARSER_H

#include "tblgen/Lex/Lexer.h"
#include "tblgen/Basic/IdentifierInfo.h"
#include "tblgen/Message/DiagnosticsEngine.h"
#include "tblgen/TableGen.h"

#include <llvm/Support/Allocator.h>

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
          llvm::MemoryBuffer &Buf,
          unsigned sourceId);

   ~Parser();

   bool parse();

   RecordKeeper &getRK()
   {
      return *GlobalRK;
   }

private:
   TableGen &TG;
   lex::Lexer lex;
   Class *currentClass = nullptr;

   std::unique_ptr<RecordKeeper> GlobalRK;
   RecordKeeper *RK;

   lex::Lexer::LookaheadRAII *LR = nullptr;
   llvm::StringMap<Value*> ForEachVals;

   LLVM_ATTRIBUTE_NORETURN
   void abortBP();

   void parseNextDecl();
   void parseRecordLevelDecl(Record *R);
   void parseClassLevelDecl(Class *C);

   void parseClass();
   void parseTemplateParams(Class *C,
                            llvm::SmallVectorImpl<size_t> &fieldParameters);
   void parseBases(Class *C);
   void parseFieldDecl(Class *C);
   void parseOverrideDecl(Class *C);

   void parseRecord();
   void parseBases(Record *R);
   void parseFieldDef(Record *R);

   void finalizeRecord(Record &R);
   void validateTemplateArgs(Class &C,
                             llvm::SmallVectorImpl<SourceLocation> &locs,
                             std::vector<Value*> &givenParams);

   void parseEnum();
   void parseEnumCase(Enum *E);

   void parseValue();

   void parseInclude();
   void parseIf(Class *C = nullptr, Record *R = nullptr);
   void parseForEach(Class *C = nullptr, Record *R = nullptr);
   void parsePrint();
   void parseNamespace();

   llvm::StringRef tryParseIdentifier();

   Type *parseType();
   Value *parseExpr(Type *contextualTy = nullptr);
   Value *parseFunction(Type *contextualTy = nullptr);

   void parseTemplateArgs(std::vector<Value*> &args,
                          llvm::SmallVectorImpl<SourceLocation> &locs,
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
      ForEachScope(Parser &P, llvm::StringRef name, Value *V)
         : P(P), name(name)
      {
         P.ForEachVals.try_emplace(name, V);
      }

      ~ForEachScope()
      {
         P.ForEachVals.erase(name);
      }

   private:
      Parser &P;
      llvm::StringRef name;
   };

   Value *getForEachVal(llvm::StringRef name)
   {
      auto it = ForEachVals.find(name);
      if (it != ForEachVals.end())
         return it->getValue();

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

} // namespace tblgen


#endif //TABLEGEN_PARSER_H
