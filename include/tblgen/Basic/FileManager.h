//
// Created by Jonas Zell on 14.10.17.
//

#ifndef TBLGEN_FILEMANAGER_H
#define TBLGEN_FILEMANAGER_H

#include "tblgen/Lex/SourceLocation.h"

#include <llvm/ADT/DenseMap.h>
#include <llvm/ADT/StringMap.h>
#include <llvm/ADT/Twine.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/raw_ostream.h>

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
   OpenFile(llvm::StringRef FileName = "",
            SourceID SourceId = 0,
            unsigned int BaseOffset = 0,
            llvm::MemoryBuffer *Buf = nullptr)
      : FileName(FileName), SourceId(SourceId), BaseOffset(BaseOffset), Buf(Buf)
   { }

   llvm::StringRef FileName;
   SourceID SourceId;
   SourceOffset BaseOffset;
   llvm::MemoryBuffer *Buf;
};

class FileManager {
public:
   FileManager();

   OpenFile openFile(const llvm::Twine &fileName, bool CreateSourceID = true);
   OpenFile getBufferForString(llvm::StringRef Str);

   OpenFile getOpenedFile(SourceID sourceId);
   OpenFile getOpenedFile(SourceLocation loc)
   { return getOpenedFile(getSourceId(loc)); }

   llvm::MemoryBuffer *getBuffer(SourceID sourceId);
   llvm::MemoryBuffer *getBuffer(SourceLocation loc)
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

   llvm::StringRef getFileName(SourceLocation loc)
   {
      return getFileName(getSourceId(loc));
   }

   llvm::StringRef getFileName(SourceID sourceId);

   LineColPair getLineAndCol(SourceLocation loc);
   LineColPair getLineAndCol(SourceLocation loc, llvm::MemoryBuffer *Buf);
   llvm::ArrayRef<SourceOffset> getLineOffsets(SourceID sourceID);

   struct CachedFile {
      CachedFile(std::string &&FN,
                 SourceID SourceId,
                 SourceOffset BaseOffset,
                 std::unique_ptr<llvm::MemoryBuffer> &&Buf)
         : FileName(move(FN)), SourceId(SourceId), BaseOffset(BaseOffset),
           Buf(move(Buf)), IsMacroExpansion(false), IsMixin(false)
      { }

      std::string FileName;
      SourceID SourceId;
      SourceOffset BaseOffset;
      std::unique_ptr<llvm::MemoryBuffer> Buf;

      bool IsMacroExpansion : 1;
      bool IsMixin          : 1;

      llvm::StringMapEntry<CachedFile> *IncludedFrom = nullptr;
   };

   const llvm::StringMap<CachedFile> &getSourceFiles() const
   { return MemBufferCache; }

   void dumpSourceLine(SourceLocation Loc);
   void dumpSourceRange(SourceRange Loc);

private:
   std::vector<SourceOffset> sourceIdOffsets;
   llvm::StringMap<CachedFile> MemBufferCache;
   llvm::DenseMap<SourceID, llvm::StringMapEntry<CachedFile>*> IdFileMap;

   std::unordered_map<SourceID, SourceLocation> aliases;
   std::unordered_map<SourceID, std::vector<SourceOffset>> LineOffsets;

   const std::vector<SourceOffset> &collectLineOffsetsForFile(SourceID sourceId,
                                                       llvm::MemoryBuffer *Buf);

   llvm::DenseMap<SourceID, SourceLocation> Imports;
};

using SourceFileRef = llvm::StringMapEntry<FileManager::CachedFile>*;

} // namespace fs
} // namespace tblgen

#endif //TBLGEN_FILEMANAGER_H
