
#ifndef TBLGEN_IDENTIFIERINFO_H
#define TBLGEN_IDENTIFIERINFO_H

#include "tblgen/Lex/TokenKinds.h"
#include "tblgen/Support/Allocator.h"

#include <cstring>
#include <string>
#include <unordered_map>

namespace tblgen {

namespace ast {
   class ASTContext;
}

namespace diag {
   class DiagnosticBuilder;
}

class IdentifierTable;

class IdentifierInfo {
public:
   friend class IdentifierTable;

   const std::string &getIdentifier() const
   {
      return Ident;
   }

   lex::tok::TokenType getKeywordTokenKind() const
   {
      return keywordTokenKind;
   }

   template <std::size_t StrLen>
   bool isStr(const char (&Str)[StrLen]) const
   {
      return getLength() == StrLen - 1
             && memcmp(getNameStart(), Str, StrLen - 1) == 0;
   }

   unsigned getLength() const
   {
      return (unsigned)Ident.size();
   }

   const char *getNameStart() const
   {
      return Ident.data();
   }

private:
   IdentifierInfo()
      : keywordTokenKind(lex::tok::sentinel)
   {}

   std::string Ident;
   lex::tok::TokenType keywordTokenKind;
};

class IdentifierTable {
public:
   using AllocatorTy = support::ArenaAllocator;
   using MapTy       = std::unordered_map<std::string_view, IdentifierInfo*>;

   explicit IdentifierTable(support::ArenaAllocator &Allocator,
                            unsigned initialSize = 8192)
      : Allocator(Allocator), IdentMap(initialSize)
   {}

   IdentifierInfo &get(std::string_view key);

   IdentifierInfo &get(std::string_view key, lex::tok::TokenType kind)
   {
      auto &Info = get(key);
      Info.keywordTokenKind = kind;

      return Info;
   }

   using iterator = MapTy::const_iterator;
   using const_iterator = MapTy::const_iterator;

   [[nodiscard]] iterator begin() const { return IdentMap.begin(); }
   [[nodiscard]] iterator end()   const { return IdentMap.end(); }
   [[nodiscard]] unsigned size()  const { return IdentMap.size(); }

   void addTblGenKeywords();

private:
   support::ArenaAllocator &Allocator;
   MapTy IdentMap;

   void addKeyword(lex::tok::TokenType kind, std::string_view kw);
};

} // namespace tblgen

#endif //TBLGEN_IDENTIFIERINFO_H
