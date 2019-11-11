
#ifndef TBLGEN_FILEMANAGER_H
#define TBLGEN_FILEMANAGER_H

#include "tblgen/Lex/SourceLocation.h"
#include "tblgen/Support/Optional.h"

#include <fstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace tblgen {
namespace fs {

struct LineColPair {
   unsigned line;
   unsigned col;
};

using SourceID     = unsigned;
using SourceOffset = unsigned;

extern SourceID InvalidID;

struct OpenFile {
   OpenFile(const std::string &Buf,
            std::string_view FileName = "",
            SourceID SourceId = 0,
            unsigned int BaseOffset = 0)
      : Buf(Buf), FileName(FileName), SourceId(SourceId), BaseOffset(BaseOffset)
   { }

   const std::string &Buf;
   std::string_view FileName;
   SourceID SourceId;
   SourceOffset BaseOffset;
};

class FileManager {
public:
   FileManager();

   support::Optional<OpenFile> openFile(const std::string &fileName,
                                        bool CreateSourceID = true);

   OpenFile getOpenedFile(SourceID sourceId);
   OpenFile getOpenedFile(SourceLocation loc)
   { return getOpenedFile(getSourceId(loc)); }

   const std::string &getBuffer(SourceID sourceId);
   const std::string &getBuffer(SourceLocation loc)
   { return getBuffer(getSourceId(loc)); }

   SourceOffset getBaseOffset(SourceID sourceId)
   {
      return sourceIdOffsets[sourceId - 1];
   }

   SourceOffset getBaseOffset(SourceLocation loc)
   {
      return getBaseOffset(getSourceId(loc));
   }

   SourceID getSourceId(SourceLocation loc);
   SourceID getLexicalSourceId(SourceLocation loc);

   std::string_view getFileName(SourceLocation loc)
   {
      return getFileName(getSourceId(loc));
   }

   std::string_view getFileName(SourceID sourceId);

   LineColPair getLineAndCol(SourceLocation loc);
   LineColPair getLineAndCol(SourceLocation loc, const std::string &Buf);
   const std::vector<SourceOffset> &getLineOffsets(SourceID sourceID);

   struct CachedFile {
      CachedFile(std::string &&FN,
                 SourceID SourceId,
                 SourceOffset BaseOffset,
                 std::string &&Buf)
         : FileName(move(FN)), SourceId(SourceId), BaseOffset(BaseOffset),
           Buf(move(Buf)), IsMacroExpansion(false), IsMixin(false)
      { }

      std::string FileName;
      SourceID SourceId;
      SourceOffset BaseOffset;
      std::string Buf;

      bool IsMacroExpansion : 1;
      bool IsMixin          : 1;

      std::unordered_map<std::string, CachedFile>::iterator IncludedFrom;
   };

   const std::unordered_map<std::string, CachedFile> &getSourceFiles() const
   { return MemBufferCache; }

   void dumpSourceLine(SourceLocation Loc);
   void dumpSourceRange(SourceRange Loc);

private:
   std::vector<SourceOffset> sourceIdOffsets;
   std::unordered_map<std::string, CachedFile> MemBufferCache;
   std::unordered_map<SourceID, std::unordered_map<std::string, CachedFile>::iterator> IdFileMap;

   std::unordered_map<SourceID, SourceLocation> aliases;
   std::unordered_map<SourceID, std::vector<SourceOffset>> LineOffsets;

   const std::vector<SourceOffset> &collectLineOffsetsForFile(
      SourceID sourceId, const std::string &Buf);

   std::unordered_map<SourceID, SourceLocation> Imports;
};

using SourceFileRef = std::unordered_map<std::string, FileManager::CachedFile>::iterator;

} // namespace fs
} // namespace tblgen

#endif //TBLGEN_FILEMANAGER_H
