
#include "tblgen/Basic/FileManager.h"
#include "tblgen/Basic/FileUtils.h"

#include <iostream>

using std::string;

namespace tblgen {
namespace fs {

SourceID InvalidID = SourceID(-1);

FileManager::FileManager() : sourceIdOffsets{1} {}

support::Optional<OpenFile>
FileManager::openFile(const std::string &name, bool CreateSourceID)
{
   auto it = MemBufferCache.find(name);
   if (it != MemBufferCache.end()) {
      auto &File = it->second;
      return OpenFile(File.Buf, File.FileName, File.SourceId, File.BaseOffset);
   }

   std::ifstream ifs(name);
   if (ifs.fail()) {
      return support::None;
   }

   SourceID id;
   SourceID previous;
   std::string buf((std::istreambuf_iterator<char>(ifs)),
                   std::istreambuf_iterator<char>());

   std::unordered_map<std::string, CachedFile>::iterator Entry;
   if (CreateSourceID) {
      previous = sourceIdOffsets.back();
      id = (unsigned)sourceIdOffsets.size();

      auto offset = unsigned(previous + buf.size());

      CachedFile file(string(name), id, previous, move(buf));
      Entry  = MemBufferCache.emplace(name, std::move(file)).first;

      IdFileMap.try_emplace(id, Entry);
      sourceIdOffsets.push_back(offset);
   }
   else {
      previous = 0;
      id = 0;

      CachedFile file(string(name), 0, 0, move(buf));
      Entry = MemBufferCache.emplace(name, std::move(file)).first;
   }

   return OpenFile(Entry->second.Buf, Entry->second.FileName, id, previous);
}

OpenFile FileManager::getOpenedFile(SourceID sourceId)
{
   auto index = IdFileMap.find(sourceId);
   assert(index != IdFileMap.end());

   auto &F = index->second->second;
   return OpenFile(F.Buf, F.FileName, F.SourceId, F.BaseOffset);
}

const std::string &FileManager::getBuffer(SourceID sourceId)
{
   auto index = IdFileMap.find(sourceId);
   assert(index != IdFileMap.end());

   return index->second->second.Buf;
}

unsigned FileManager::getSourceId(SourceLocation loc)
{
   if (loc.getOffset() == 0)
      return 0;

   if (sourceIdOffsets.size() == 1)
      return 1;

   unsigned needle = loc.getOffset();
   unsigned L = 0;
   unsigned R = (unsigned)sourceIdOffsets.size() - 1;
   unsigned m;

   if (needle > sourceIdOffsets.back())
      return (unsigned)sourceIdOffsets.size() - 1;

   while (true) {
      m = (L + R) / 2u;
      if (L > R || sourceIdOffsets[m] == needle)
         break;

      if (sourceIdOffsets[m] < needle)
         L = m + 1;
      else
         R = m - 1;
   }

   return m + 1;
}

std::string_view FileManager::getFileName(SourceID sourceId)
{
   auto index = IdFileMap.find(sourceId);
   assert (index != IdFileMap.end() && "invalid source ID!");

   return index->second->first;
}

LineColPair FileManager::getLineAndCol(SourceLocation loc)
{
   if (!loc)
      return {0, 0};

   auto file = getBuffer(loc);
   return getLineAndCol(loc, file);
}

LineColPair FileManager::getLineAndCol(SourceLocation loc,
                                       const std::string &Buf)
{
   auto ID = getSourceId(loc);
   auto it = LineOffsets.find(ID);
   auto const &offsets = it == LineOffsets.end()
                             ? collectLineOffsetsForFile(ID, Buf)
                             : it->second;

   assert(!offsets.empty());

   unsigned needle = loc.getOffset() - getBaseOffset(ID);
   unsigned L = 0;
   unsigned R = (unsigned)offsets.size() - 1;
   unsigned m;

   while (true) {
      m = (L + R) / 2u;
      if (L > R || offsets[m] == needle)
         break;

      if (offsets[m] < needle)
         L = m + 1;
      else
         R = m - 1;
   }

   auto closestOffset = offsets[m];
   assert(closestOffset <= needle);

   // special treatment for first line
   if (!m)
      ++needle;

   return {m + 1, needle - closestOffset + 1};
}

const std::vector<unsigned> &FileManager::getLineOffsets(SourceID sourceID)
{
   auto it = LineOffsets.find(sourceID);
   if (it == LineOffsets.end()) {
      return collectLineOffsetsForFile(sourceID, getBuffer(sourceID));
   }

   return it->second;
}

const std::vector<unsigned> &
FileManager::collectLineOffsetsForFile(SourceID sourceId,
                                       const std::string &Buf)
{
   std::vector<unsigned> newLines{0};

   unsigned idx = 0;
   auto buf = Buf.c_str();
   auto size = Buf.size();

   while (idx < size) {
      switch (*buf) {
      case '\n':
         newLines.push_back(idx);
         break;
      default:
         break;
      }

      ++idx;
      ++buf;
   }

   return LineOffsets.emplace(sourceId, move(newLines)).first->second;
}

static bool isNewline(const char *str)
{
   switch (str[0]) {
   case '\n':
   case '\r':
      return true;
   default:
      return false;
   }
}

void FileManager::dumpSourceLine(SourceLocation Loc)
{
   return dumpSourceRange(Loc);
}

void FileManager::dumpSourceRange(SourceRange SR)
{
   auto &out = std::cout;

   auto loc = SR.getStart();
   size_t ID = getSourceId(loc);
   auto File = getOpenedFile(ID);

   const std::string &Buf = File.Buf;
   size_t srcLen = Buf.size();
   const char *src = Buf.c_str();

   // show file name, line number and column
   auto lineAndCol = getLineAndCol(loc, Buf);
   out << "(" << fs::getFileNameAndExtension(File.FileName) << ":"
       << lineAndCol.line << ":" << lineAndCol.col << ")\n";

   unsigned errLineNo = lineAndCol.line;

   // only source ranges that are on the same line as the "main index" are shown
   unsigned errIndex = loc.getOffset() - File.BaseOffset;
   unsigned newlineIndex = errIndex;

   // find offset of first newline before the error index
   for (; newlineIndex > 0; --newlineIndex) {
      if (isNewline(src + newlineIndex))
         break;
   }

   // find offset of first newline after error index
   unsigned lineEndIndex = errIndex;
   for (; lineEndIndex < srcLen; ++lineEndIndex) {
      if (isNewline(src + lineEndIndex))
         break;
   }

   unsigned Len;
   if (lineEndIndex == newlineIndex) {
      Len = 0;
   }
   else {
      Len = lineEndIndex - newlineIndex - 1;
   }

   std::string_view ErrLine(Buf.c_str() + newlineIndex + 1, Len);

   // show carets for any given single source location, and tildes for source
   // ranges (but only on the error line)
   std::string Markers;

   Markers.resize(ErrLine.size());
   std::fill(Markers.begin(), Markers.end(), ' ');

   auto Start = SR.getStart();
   auto End = SR.getEnd();

   do {
      // single source location, show caret
      if (!End) {
         unsigned offset = Start.getOffset() - File.BaseOffset;
         if (lineEndIndex <= offset || newlineIndex >= offset) {
            // source location is on a different line
            break;
         }

         unsigned offsetOnLine = offset - newlineIndex - 1;
         Markers[offsetOnLine] = '^';
      }
      else {
         auto BeginOffset = Start.getOffset() - File.BaseOffset;
         auto EndOffset = End.getOffset() - File.BaseOffset;

         unsigned BeginOffsetOnLine = BeginOffset - newlineIndex - 1;
         unsigned EndOffsetOnLine
             = std::min(EndOffset, lineEndIndex) - newlineIndex - 1;

         if (EndOffsetOnLine > lineEndIndex)
            break;

         assert(EndOffsetOnLine >= BeginOffsetOnLine
                && "invalid source range!");

         while (1) {
            Markers[EndOffsetOnLine] = '~';
            if (EndOffsetOnLine-- == BeginOffsetOnLine)
               break;
         }

         Markers[BeginOffsetOnLine] = '^';
      }
   } while (0);

   // display line number to the left of the source
   std::string LinePrefix;
   LinePrefix += std::to_string(errLineNo);
   LinePrefix += " | ";

   out << LinePrefix << ErrLine << "\n"
       << std::string(LinePrefix.size(), ' ') << Markers << "\n";
}

} // namespace fs
} // namespace tblgen