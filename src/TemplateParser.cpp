
#include "tblgen/Parser.h"

#include "tblgen/Record.h"
#include "tblgen/TableGen.h"
#include "tblgen/Value.h"
#include "tblgen/Basic/FileManager.h"
#include "tblgen/Basic/FileUtils.h"
#include "tblgen/Support/Casting.h"
#include "tblgen/Support/LiteralParser.h"
#include "tblgen/Support/SaveAndRestore.h"

#include <sstream>

using namespace tblgen::lex;
using namespace tblgen::diag;
using namespace tblgen::support;

using std::string;

namespace tblgen {

TemplateParser::TemplateParser(tblgen::TableGen &TG, const std::string &Buf,
                               unsigned sourceId, unsigned baseOffset)
                               : Parser(TG, Buf, sourceId, baseOffset)
{
   ActiveOS = &OS;
}

TemplateParser::TemplateParser(tblgen::TableGen &TG, const std::vector<Token> &Toks,
                               unsigned sourceId, unsigned baseOffset)
   : Parser(TG, Toks, sourceId, baseOffset)
{
   ActiveOS = &OS;
}

bool TemplateParser::parseTemplate()
{
   while (!currentTok().is(tok::eof)) {
      if (!commandFollows()) {
         currentTokens.push_back(currentTok());
         advanceNoSkip();

         continue;
      }

      if (!currentTokens.empty()) {
         appendTokens(currentTok().getIdentifierInfo()->isStr("<%"));
      }

      handleTemplateExpr();
      advanceNoSkip();
   }

   if (!currentTokens.empty()) {
      appendTokens(false);
   }

   return TG.Diags.getNumErrors() == 0;
}

void TemplateParser::appendTokens(bool beforeCommand)
{
   int remove = 0;
   bool foundNewline = false;

   if (beforeCommand) {
      for (int i = currentTokens.size() - 1; i >= 0; --i) {
         if (currentTokens[i].is(tok::newline)) {
            foundNewline = true;
            ++remove;

            break;
         }

         if (currentTokens[i].isWhitespace()) {
            ++remove;
         }
         else {
            break;
         }
      }
   }

   if (!foundNewline) {
      remove = 0;
   }

   for (int i = 0; i < currentTokens.size() - remove; ++i) {
      *ActiveOS << currentTokens[i].rawRepr();
   }

   currentTokens.clear();
}

std::string TemplateParser::getResult()
{
   return OS.str();
}

void TemplateParser::parseUntilEnd(bool *isElse)
{
   while (!currentTok().is(tok::eof)) {
      if (!commandFollows()) {
         currentTokens.push_back(currentTok());
         advanceNoSkip();

         continue;
      }

      if (!currentTokens.empty()) {
         appendTokens(currentTok().getIdentifierInfo()->isStr("<%"));
      }

      bool isEnd = false;
      handleTemplateExpr(&isEnd, isElse);

      if (isEnd || (isElse && *isElse)) {
         break;
      }

      advanceNoSkip();
   }

   if (!currentTokens.empty()) {
      appendTokens(false);
   }
}

void TemplateParser::skipUntilEnd(std::vector<lex::Token> *tokens)
{
   int openEnds = 1;
   while (!currentTok().is(tok::eof)) {
      if (tokens)
         tokens->push_back(currentTok());

      if (!commandFollows()) {
         advanceNoSkip();
         continue;
      }

      advance();

      if (currentTok().isIdentifier("end")) {
         if (tokens && openEnds > 1)
            tokens->push_back(currentTok());

         advance();
         if (--openEnds == 0) {
            break;
         }
      }
      else if (currentTok().is(tok::tblgen_if)
      || currentTok().isIdentifier("for_each")
      || currentTok().isIdentifier("for_each_record")
      || currentTok().isIdentifier("for_each_join")
      || currentTok().isIdentifier("define")) {
         ++openEnds;
      }
   }

   // Remove the '<%' from '<% end'
   if (tokens && !tokens->empty() && tokens->back().is(tok::op_ident))
      tokens->pop_back();
}

bool TemplateParser::commandFollows()
{
   return currentTok().is(tok::op_ident)
      && (currentTok().getIdentifierInfo()->isStr("<%")
         || currentTok().getIdentifierInfo()->isStr("<%%"));
}

static bool isParameterlessCommand(const IdentifierInfo *ident)
{
   return ident->isStr("end")
      || ident->isStr("else")
      || ident->isStr("invoke")
      || ident->isStr("define");
}

void TemplateParser::handleTemplateExpr(bool *isEnd, bool *isElse)
{
   auto openParenLoc = currentTok().getSourceLoc();
   assert(currentTok().is(tok::op_ident));

   // Check if this is a paste command
   bool paste = currentTok().getIdentifierInfo()->isStr("<%%");
   advance();

   // Check if it is just an expression.
   bool isCommand = currentTok().oneOf(tok::ident, tok::tblgen_if)
                    && (peek().is(tok::op_or)
                    || isParameterlessCommand(currentTok().getIdentifierInfo()));

   const Value *result = nullptr;
   bool explicitStr = false;

   if (isCommand) {
      result = handleCommand(paste, explicitStr, isEnd, isElse);
   }
   else {
      result = parseExpr();
      advance();
   }

   if (paste) {
      if (!result) {
         TG.Diags.Diag(warn_generic_warn)
            << "expression does not produce code to paste"
            << openParenLoc;
      }
      else {
         if (!explicitStr && isa<StringLiteral>(result)) {
            *ActiveOS << cast<StringLiteral>(result)->getVal();
         }
         else {
            *ActiveOS << result;
         }
      }
   }

   if ((paste && !currentTok().isIdentifier("%%>")) || (!paste && !currentTok().isIdentifier("%>"))) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + currentTok().toString() + ", expecting '%>'"
         << lex.getSourceLoc();

      abortBP();
   }
}

#define EXPECT_NUM_ARGS(ArgCnt)                                           \
   if (args.size() != ArgCnt) { TG.Diags.Diag(err_generic_error)          \
      << "function " + func + " expects " + std::to_string(ArgCnt)        \
         + " arguments" << parenLoc; abortBP(); }

#define EXPECT_AT_LEAST_ARGS(ArgCnt)                                      \
   if (args.size() < ArgCnt) { TG.Diags.Diag(err_generic_error)           \
      << "function " + func + " expects at least "                        \
         + std::to_string(ArgCnt)                                         \
         + " arguments" << parenLoc; abortBP(); }

#define EXPECT_ARG_VALUE(ArgNo, ValKind)                                \
   if (!isa<ValKind>(args[ArgNo])) { TG.Diags.Diag(err_generic_error)   \
      << "function " + func + " expects arg #" + std::to_string(ArgNo)  \
         + " to be a " #ValKind; abortBP(); }

Value* TemplateParser::handleCommand(bool paste, bool &explicitStr,
                                     bool *isEnd, bool *isElse)
{
   const IdentifierInfo *cmd;
   if (currentTok().is(tok::tblgen_if)) {
      cmd = &TG.getIdents().get("if");
   }
   else {
      cmd = currentTok().getIdentifierInfo();
   }

   auto parenLoc = currentTok().getSourceLoc();

   if (cmd->isStr("for_each")
   || cmd->isStr("for_each_record")
   || cmd->isStr("for_each_join")) {
      return handleForeach(paste);
   }

   if (cmd->isStr("if")) {
      return handleIf(paste);
   }

   if (cmd->isStr("define")) {
      return handleDefine(paste);
   }

   if (cmd->isStr("invoke")) {
      return handleInvoke(paste);
   }

   std::vector<Value*> args;
   if (peek().is(tok::op_or)) {
      advance();
      advance();

      while (true) {
         auto *argVal = parseExpr();
         args.push_back(argVal);

         if (peek().is(tok::comma)) {
            advance();
            advance();

            continue;
         }

         break;
      }
   }

   const std::string &func = cmd->getIdentifier();
   advance();

   if (cmd->isStr("end")) {
      if (isEnd == nullptr) {
         TG.Diags.Diag(err_generic_error)
            << "command 'end' is not allowed here"
            << lex.getSourceLoc();
      }
      else {
         *isEnd = true;
      }

      return nullptr;
   }

   if (cmd->isStr("else")) {
      if (isElse == nullptr) {
         TG.Diags.Diag(err_generic_error)
            << "command 'else' is not allowed here"
            << lex.getSourceLoc();
      }
      else {
         *isElse = true;
      }

      return nullptr;
   }

   if (cmd->isStr("str")) {
      EXPECT_NUM_ARGS(1)

      explicitStr = true;

      if (isa<StringLiteral>(args.front())) {
         return args.front();
      }

      std::ostringstream str;
      str << args.front();

      return new(TG) StringLiteral(TG.getStringTy(), str.str());
   }

   if (cmd->isStr("record_name")) {
      EXPECT_NUM_ARGS(1)
      EXPECT_ARG_VALUE(0, RecordVal)

      return new(TG) IdentifierVal(
         TG.getStringTy(),
         cast<RecordVal>(args.front())->getRecord()->getName());
   }

   TG.Diags.Diag(err_generic_error)
      << "unknown command '" + cmd->getIdentifier() + "'"
      << lex.getSourceLoc();

   return nullptr;
}

Value* TemplateParser::handleIf(bool paste)
{
   if (!peek().is(tok::op_or)) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + currentTok().toString() + ", expecting '|'"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();
   advance();

   auto *cond = parseExpr(TG.getInt1Ty());
   if (!cond || !isa<IntType>(cond->getType())
      || cast<IntType>(cond->getType())->getBitWidth() != 1) {
      TG.Diags.Diag(err_generic_error)
         << "'if' command expects a boolean argument"
         << lex.getSourceLoc();

      abortBP();
   }

   if ((paste && !peek().isIdentifier("%%>")) || (!paste && !peek().isIdentifier("%>"))) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + currentTok().toString() + ", expecting '%>'"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();
   advanceNoSkip();

   bool foundElse = false;
   bool condIsTrue = cast<IntegerLiteral>(cond)->getVal() > 0;

   if (!condIsTrue) {
      std::ostringstream discard;

      auto SAR = support::saveAndRestore(ActiveOS, &discard);
      parseUntilEnd(&foundElse);
   }
   else {
      parseUntilEnd(&foundElse);
   }

   if (foundElse)
   {
      advanceNoSkip();

      if (!condIsTrue) {
         parseUntilEnd();
      }
      else {
         std::ostringstream discard;
         auto SAR = support::saveAndRestore(ActiveOS, &discard);
         parseUntilEnd();
      }
   }

   return nullptr;
}

Value* TemplateParser::handleForeach(bool paste)
{
   enum class ForEachType {
      Normal,
      Record,
      Join,
   };

   ForEachType type;
   if (currentTok().isIdentifier("for_each_record")) {
      type = ForEachType::Record;
   }
   else if (currentTok().isIdentifier("for_each_join")) {
      type = ForEachType::Join;
   }
   else {
      type = ForEachType::Normal;
   }

   if (!peek().is(tok::op_or)) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + currentTok().toString() + ", expecting '|'"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();
   advance();

   auto *iterator = parseExpr();
   std::vector<Value*> values;

   if (type == ForEachType::Record) {
      if (!isa<StringLiteral>(iterator)) {
         TG.Diags.Diag(err_generic_error)
            << "for_each_record expects a string literal"
            << lex.getSourceLoc();

         abortBP();
      }

      std::vector<Record*> records;
      RK->getAllDefinitionsOf(cast<StringLiteral>(iterator)->getVal(), records);

      for (auto *R : records) {
         values.push_back(new(TG) RecordVal(TG.getRecordType(R), R));
      }
   }
   else {
      if (!isa<ListLiteral>(iterator)) {
         TG.Diags.Diag(err_generic_error)
            << "for_each expects an iterator of list type"
            << lex.getSourceLoc();

         abortBP();
      }

      values = cast<ListLiteral>(iterator)->getValues();
   }

   if (!peek().isIdentifier("as")) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + currentTok().toString() + ", expecting 'as'"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();

   if (!peek().is(tok::ident)) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + currentTok().toString() + ", expecting identifier"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();
   auto *ident = currentTok().getIdentifierInfo();

   Value *separator = nullptr;
   if (type == ForEachType::Join) {
      if (!peek().is(tok::comma)) {
         separator = new(TG) StringLiteral(TG.getStringTy(), ", ");
      }
      else {
         advance();
         advance();

         separator = parseExpr(TG.getStringTy());
      }

      if (!isa<StringType>(separator->getType())) {
         TG.Diags.Diag(err_generic_error)
            << "for_each_join separator must be a string"
            << lex.getSourceLoc();

         abortBP();
      }
   }

   IdentifierInfo *iterName = nullptr;
   if (peek().is(tok::comma)) {
      advance();

      if (!peek().is(tok::ident)) {
         TG.Diags.Diag(err_generic_error)
            << "unexpected token " + currentTok().toString() + ", expecting identifier"
            << lex.getSourceLoc();

         abortBP();
      }

      advance();
      iterName = currentTok().getIdentifierInfo();
   }

   if ((paste && !peek().isIdentifier("%%>")) || (!paste && !peek().isIdentifier("%>"))) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + currentTok().toString() + ", expecting '%>'"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();
   advanceNoSkip();

   int i = 0;
   for (auto *V : values) {
      if (type == ForEachType::Join) {
         if (i != 0) {
            OS << cast<StringLiteral>(separator)->getVal();
         }
      }

      Lexer::LookaheadRAII LR(lex);

      auto SAR = support::saveAndRestore(this->LR, &LR);
      ForEachScope scope(*this, ident->getIdentifier(), V);

      if (iterName != nullptr) {
         ForEachVals[iterName->getIdentifier()] =
            new(TG) IntegerLiteral(TG.getInt64Ty(), (uint64_t)i);
      }

      parseUntilEnd();
      ++i;
   }

   if (iterName != nullptr && !values.empty()) {
      ForEachVals.erase(ForEachVals.find(iterName->getIdentifier()));
   }

   skipUntilEnd();
   return nullptr;
}

Value* TemplateParser::handleDefine(bool paste)
{
   if (!peek().is(tok::ident)) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + peek().toString() + ", expecting macro name"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();

   Macro macro;
   std::string name(currentTok().getIdentifier());

   if (peek().is(tok::open_paren)) {
      advance();
      advance();

      while (!currentTok().is(tok::close_paren)) {
         if (!currentTok().is(tok::ident)) {
            TG.Diags.Diag(err_generic_error)
               << "unexpected token " + currentTok().toString() + ", expecting parameter name"
               << lex.getSourceLoc();

            abortBP();
         }

         macro.params.emplace_back(currentTok().getIdentifier());

         advance();
         if (currentTok().is(tok::comma))
            advance();
      }
   }

   if ((paste && !peek().isIdentifier("%%>")) || (!paste && !peek().isIdentifier("%>"))) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + currentTok().toString() + ", expecting '%>'"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();
   advanceNoSkip();

   skipUntilEnd(&macro.tokens);
   macros[name] = macro;

   return nullptr;
}

Value* TemplateParser::handleInvoke(bool paste)
{
   if (!peek().is(tok::ident)) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + peek().toString() + ", expecting macro name"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();
   std::string name(currentTok().getIdentifier());

   if (macros.find(name) == macros.end())
   {
      TG.Diags.Diag(err_generic_error)
         << "macro " + name + " is not defined"
         << lex.getSourceLoc();

      abortBP();
   }

   auto &macro = macros[name];
   std::vector<Value*> args;

   if (peek().is(tok::open_paren)) {
      advance();
      advance();

      while (!currentTok().is(tok::close_paren)) {
         args.push_back(parseExpr());
         advance();

         if (currentTok().is(tok::comma))
            advance();
      }
   }

   if (args.size() != macro.params.size())
   {
      TG.Diags.Diag(err_generic_error)
         << "macro " + name + " expects " + std::to_string(macro.params.size())
            + " arguments, " + std::to_string(args.size()) + " given"
         << lex.getSourceLoc();

      abortBP();
   }

   if ((paste && !peek().isIdentifier("%%>")) || (!paste && !peek().isIdentifier("%>"))) {
      TG.Diags.Diag(err_generic_error)
         << "unexpected token " + currentTok().toString() + ", expecting '%>'"
         << lex.getSourceLoc();

      abortBP();
   }

   advance();

   TemplateParser parser(TG, macro.tokens, this->lex.getSourceId(), this->lex.getOffset());
   for (int i = 0; i < macro.params.size(); ++i)
   {
      parser.ForEachVals[macro.params[i]] = args[i];
   }

   if (parser.parseTemplate())
   {
      *ActiveOS << parser.getResult();
   }

   return nullptr;
}

} // namespace tblgen