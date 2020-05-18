
#ifndef LEXER_H
#define LEXER_H

#include "tblgen/Message/Diagnostics.h"
#include "tblgen/Lex/Token.h"

#include <string>
#include <vector>

namespace tblgen {

class IdentifierTable;

namespace diag {
   class DiagnosticBuilder;
} // namespace diag

namespace parse {
   class Parser;
} // namespace parse

namespace lex {

class Lexer {
public:
   using TokenVec   = std::vector<Token>;
   using PPTokenVec = std::vector<Token>;

   Lexer(IdentifierTable &Idents,
         DiagnosticsEngine &Diags,
         const std::string &buf,
         unsigned sourceId,
         unsigned offset = 1,
         const char InterpolationBegin = '$',
         bool primeLexer = true);


   Lexer(IdentifierTable &Idents,
         DiagnosticsEngine &Diags,
         const std::vector<Token> &Tokens,
         unsigned sourceId,
         unsigned offset = 1);

#ifndef NDEBUG
   virtual
#endif
   ~Lexer() = default;

   void reset(const std::vector<Token> &Tokens);

   void lexDiagnostic();
   void lexStringInterpolation();

   [[nodiscard]] std::string_view getCurrentIdentifier() const;

   enum class Mode : int {
      /// No special lexing required.
      Normal,

      /// Always lex '<' and '>' as single tokens.
      ParsingTemplateParams,

      /// Always lex '>' as a single token.
      ParsingTemplateArgs,
   };

   struct ModeRAII {
      ModeRAII(Lexer &L, Mode M) : L(L), Prev(L.CurMode)
      {
         L.CurMode = M;
      }

      ~ModeRAII()
      {
         L.CurMode = Prev;
      }

   private:
      Lexer &L;
      Mode Prev;
   };

   struct LookaheadRAII {
      explicit LookaheadRAII(Lexer &L);
      ~LookaheadRAII();

      void advance(bool ignoreNewline = true,
                   bool significantWhitespace = false);

   private:
      Lexer &L;
      std::vector<Token> Tokens;

      Token LastTok;
      Token CurTok;
   };

   template<class ...Rest>
   void expect(tok::TokenType ty, Rest... rest)
   {
      advance();
      expect_impl(ty, rest...);
   }

   template<class ...Rest>
   void expect_impl(tok::TokenType ty, Rest... rest)
   {
      if (currentTok().is(ty)) return;
      expect_impl(rest...);
   }

   void expect_impl(tok::TokenType ty);

   template <class ...Rest>
   bool advanceIf(tok::TokenType ty, Rest... rest)
   {
      if (currentTok().is(ty)) {
         advance();
         return true;
      }
      else {
         return advanceIf(rest...);
      }
   }

   bool advanceIf(tok::TokenType ty)
   {
      if (currentTok().is(ty)) {
         advance();
         return true;
      }

      return false;
   }

   void advance(bool ignoreNewline = true,
                bool significantWhitespace = false,
                bool rememberTok = true);

   Token lookahead(bool ignoreNewline = true, bool sw = false, size_t i = 0);
   void backtrack();

   static char escape_char(char);
   static std::string unescape_char(char);

   const char* getSrc() { return BufStart; }
   const char* getBuffer() { return CurPtr; }
   [[nodiscard]] unsigned int getSourceId() const { return sourceId; }
   [[nodiscard]] unsigned int getOffset() const { return offset; }

   [[nodiscard]] unsigned currentIndex() const { return unsigned(CurPtr - BufStart); }

   [[nodiscard]] Token const& currentTok() const { return CurTok; }
   [[nodiscard]] SourceLocation getSourceLoc() const { return CurTok.getSourceLoc(); }
   [[nodiscard]] IdentifierTable &getIdents() const { return Idents; }

   void printTokensTo(std::ostream &out);
   void dump();

   [[nodiscard]] bool eof() const { return AtEOF; }
   [[nodiscard]] bool interpolation() const { return InInterpolation; }

   void insertLookaheadTokens(const std::vector<Token> &Toks)
   {
      LookaheadVec.insert(LookaheadVec.end(), Toks.begin(), Toks.end());
   }

   void setCurTok(Token t) { this->CurTok = t; }
   void setLastTok(Token t) { this->LastTok = t; }
   Token getLastTok() const { return LastTok; }

   enum ParenKind {
      PAREN = '(',
      BRACE = '{',
      ANGLED = '<',
      SQUARE = '['
   };

   friend diag::DiagnosticBuilder& operator<<(diag::DiagnosticBuilder &builder,
                                              Lexer* const& lex) {
      builder.setLoc(lex->getSourceLoc());
      return builder;
   }

   friend class parse::Parser;

protected:
   Token lexStringLiteral();
   Token lexNumericLiteral();
   Token lexCharLiteral();

   bool isIdentifierContinuationChar(char c);
   Token lexIdentifier(tok::TokenType = tok::ident, bool AllowMacro = true);

   Token lexClosureArgumentName();

   void lexOperator();
   void lexPreprocessorExpr();
   Token skipSingleLineComment();
   Token skipMultiLineComment();

   tok::TokenType getBuiltinOperator(std::string_view str);

   template<class ...Args>
   Token makeToken(Args&&... args)
   {
      SourceLocation loc(TokBegin - BufStart + offset);
      return Token(args..., loc);
   }

   Token makeEOF();

   IdentifierTable &Idents;
   DiagnosticsEngine &Diags;

   /// The current lexing mode, used to lex certain characters differently in
   /// different situations.
   Mode CurMode = Mode::Normal;

   /// The previous token, backtracking by one token is supported.
   Token LastTok;

   /// The current token.
   Token CurTok;

   /// Index into the lookahead vector.
   unsigned LookaheadIdx = 0;

   /// Vector for tokens that we already lexed via lookahead or some other
   /// operation.
   std::vector<Token> LookaheadVec;

   /// ID of the source file we're lexing.
   unsigned sourceId = 0;

   /// Pointer to the current character.
   const char *CurPtr;

   /// Pointer to the beginning of the token we're currently lexing.
   const char *TokBegin;

   /// Pointer to the beginning of the buffer.
   const char *BufStart;

   /// Pointer to the end of the buffer.
   const char *BufEnd;

   /// Character that starts a string interpolation, or NUL if interpolation
   /// is disabled.
   const char InterpolationBegin;

   /// True once we reached the end of the file.
   bool AtEOF = false;

   /// Base offset of the source file.
   unsigned offset = 0;

   /// True if this lexer is operating on a prelexed list of tokens.
   bool IsTokenLexer = false;

   /// True if we're currently parsing a string interpolation.
   bool InInterpolation = false;

   Token lexNextToken();
};

} // namespace lex
} // namespace tblgen

#endif //LEXER_H
