//
// Created by Jonas Zell on 13.10.17.
//

#include "tblgen/Basic/FileUtils.h"

#include <llvm/ADT/SmallString.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Support/Path.h>
#include <llvm/Support/Program.h>

#include <cstdio>
#include <system_error>

#if defined(__APPLE__)
#  include <pwd.h>
#  include <unistd.h>
#endif

#ifdef _WIN32
#  include <direct.h>
#endif

using std::string;

namespace tblgen {
namespace fs {

std::string_view getPath(std::string_view fullPath)
{
   auto period = fullPath.rfind('.');
   auto slash = fullPath.rfind(PathSeperator);

   if (period == string::npos || (period < slash && slash != string::npos)) {
      return fullPath;
   }

   if (slash == string::npos) {
      return "";
   }

   return fullPath.substr(0, slash + 1);
}

std::string_view getFileName(std::string_view fullPath)
{
   auto period = fullPath.rfind('.');
   auto slash = fullPath.rfind(PathSeperator);

   if (period == string::npos || period < slash)
      return fullPath.substr(slash == string::npos ? 1 : slash + 1);

   if (slash == string::npos)
      return fullPath.substr(0, period);

   return fullPath.substr(slash + 1, period - slash - 1);
}

std::string_view getExtension(std::string_view fullPath)
{
   auto period = fullPath.rfind('.');
   auto slash = fullPath.rfind(PathSeperator);

   if (period == string::npos)
      return "";

   if (slash == string::npos || period > slash)
      return fullPath.substr(period + 1);

   return "";
}

std::string swapExtension(std::string_view fullPath, std::string_view newExt)
{
   llvm::SmallString<128> ScratchBuf;

   auto period = fullPath.rfind('.');
   auto slash = fullPath.rfind(PathSeperator);

   if (period == string::npos)
      return "";

   if (period < slash) {
      ScratchBuf += fullPath;
      ScratchBuf += '.';
      ScratchBuf += newExt;

      return ScratchBuf.str();
   }

   ScratchBuf += fullPath;
   ScratchBuf.resize(period + 1);
   ScratchBuf += newExt;

   return ScratchBuf.str();
}

std::string_view getFileNameAndExtension(std::string_view fullPath)
{
   auto period = fullPath.rfind('.');
   auto slash = fullPath.rfind(PathSeperator);

   if (period == string::npos)
      return "";

   if (slash == string::npos)
      return fullPath;

   if (period > slash)
      return fullPath.substr(slash + 1);

   return "";
}

bool fileExists(std::string_view name)
{
   return llvm::sys::fs::is_regular_file(name);
}

void createDirectories(std::string_view fullPath)
{
   llvm::sys::fs::create_directories(fullPath);
}

int deleteDirectory(const std::string& Dir)
{
#ifdef _WIN32
   return _rmdir(Dir.str().c_str());
#else
   return rmdir(Dir.str().c_str());
#endif
}

int deleteFile(std::string_view FileName)
{
   if (FileName.back() != '\0') {
      return std::remove(FileName.str().data());
   }
   else {
      return std::remove(FileName.data());
   }
}

namespace {

template<class iterator>
std::vector<string> getAllFilesInDirectoryImpl(std::string_view dirName)
{
   using Kind = llvm::sys::fs::file_type;

   std::vector<string> files;
   std::error_code ec;

   iterator it(dirName, ec);
   while (!ec) {
      auto &entry = *it;

      auto errOrStatus = entry.status();
      if (!errOrStatus)
         break;

      auto &st = errOrStatus.get();
      auto err = entry.status();
      if (err)
         break;

      switch (st.type()) {
         case Kind::regular_file:
         case Kind::symlink_file:
         case Kind::character_file:
            files.push_back(entry.path());
            break;
         default:
            break;
      }

      it.increment(ec);
   }

   return files;
}

} // anonymous namespace

std::vector<string> getAllFilesInDirectory(std::string_view dirName,
                                           bool recursive) {
   using llvm::sys::fs::recursive_directory_iterator;
   using llvm::sys::fs::directory_iterator;

   if (recursive)
      return getAllFilesInDirectoryImpl<recursive_directory_iterator>(dirName);

   return getAllFilesInDirectoryImpl<directory_iterator>(dirName);
}

string findFileInDirectories(std::string_view fileName,
                             const std::vector<std::string> &directories) {
   if (fileName.front() == fs::PathSeperator) {
      if (fileExists(fileName))
         return fileName;

      return "";
   }

   auto Path = fs::getPath(fileName);
   if (!Path.empty()) {
      fileName = fs::getFileNameAndExtension(fileName);
   }

   using iterator = llvm::sys::fs::directory_iterator;
   using Kind = llvm::sys::fs::file_type;

   std::error_code ec;
   iterator end_it;

   for (std::string dirName : directories) {
      if (!Path.empty()) {
         if (dirName.back() != fs::PathSeperator) {
            dirName += fs::PathSeperator;
         }

         dirName += Path;
      }

      iterator it(dirName, ec);
      while (it != end_it) {
         auto &entry = *it;

         auto errOrStatus = entry.status();
         if (!errOrStatus)
            break;

         auto &st = errOrStatus.get();
         switch (st.type()) {
            case Kind::regular_file:
            case Kind::symlink_file:
            case Kind::character_file:
               if (getFileNameAndExtension(entry.path()) == fileName.str())
                  return entry.path();

               break;
            default:
               break;
         }

         it.increment(ec);
      }

      ec.clear();
   }

   return "";
}

int executeCommand(std::string_view Program, const std::vector<string> &args)
{
   std::unique_ptr<const char*> argArray(new const char*[args.size() + 1]);
   size_t i = 0;

   while (i < args.size()) {
      argArray.get()[i] = args[i].c_str();
      ++i;
   }

   argArray.get()[i] = nullptr;

   return llvm::sys::ExecuteAndWait(Program, argArray.get());
}

long long getLastModifiedTime(std::string const& pathToFile)
{
   llvm::sys::fs::file_status stat;
   auto ec = llvm::sys::fs::status(pathToFile, stat);
   if (ec)
      return -1ll;

   return std::chrono::duration_cast<std::chrono::milliseconds>(
      stat.getLastModificationTime().time_since_epoch()).count();
}

namespace {

template <class iterator = llvm::sys::fs::directory_iterator, class Handler>
void iterateOverFilesInDirectory(std::string_view dir, Handler const& H)
{
   using Kind = llvm::sys::fs::file_type;

   std::error_code ec;
   iterator it(dir, ec);
   iterator end_it;

   while (it != end_it) {
      auto &entry = *it;

      auto errOrStatus = entry.status();
      if (!errOrStatus)
         break;

      auto &st = errOrStatus.get();
      switch (st.type()) {
         case Kind::regular_file:
         case Kind::symlink_file:
         case Kind::character_file:
            H(entry.path());
            break;
         default:
            break;
      }

      it.increment(ec);
   }
}

} // anonymous namespace

void getAllMatchingFiles(std::string_view fileName,
                         std::vector<std::string> &Out) {
   auto ext = getExtension(fileName);
   auto file = getFileName(fileName);

   if (file == "*") {
      auto path = fileName.substr(0, fileName.rfind('*'));

      // /foo/bar/* matches all files in directory, non recursively
      if (ext.empty()) {
         iterateOverFilesInDirectory(path,
                                     [&Out](string const& s) {
                                        Out.push_back(s);
                                     });
      }
      // /foo/bar/*.baz matches only files that match the extension
      else {
         iterateOverFilesInDirectory(path,
                                     [&Out, &ext](string const& s) {
                                         if (getExtension(s) == ext)
                                            Out.push_back(s);
                                     });
      }
   }
   else if (file == "**") {
      using it = llvm::sys::fs::recursive_directory_iterator;
      auto path = fileName.substr(0, fileName.rfind('*') - 1);

      // /foo/bar/** matches all files in directory, recursively
      if (ext.empty()) {
         iterateOverFilesInDirectory<it>(path,
                                         [&Out](string const& s) {
                                            Out.push_back(s);
                                         });
      }
      // /foo/bar/**.baz matches only files that match the extension
      else {
         iterateOverFilesInDirectory<it>(path,
                                         [&Out, &ext](string const& s) {
                                            if (getExtension(s) == ext)
                                               Out.push_back(s);
                                         });
      }
   }
   else {
      Out.push_back(fileName);
   }
}

void deleteAllFilesInDirectory(const std::string &Dir)
{
   using it = llvm::sys::fs::recursive_directory_iterator;

   std::vector<string> FilesToDelete;
   iterateOverFilesInDirectory<it>(Dir.str(), [&](const string &file) {
      FilesToDelete.push_back(file);
   });

   for (auto &File : FilesToDelete)
      deleteFile(File);
}

std::error_code makeAbsolute(std::vector<char> &Buf)
{
   return llvm::sys::fs::make_absolute(Buf);
}

std::string_view getLibraryDir()
{
   // FIXME
   return "/usr/local/lib";
}

std::string_view getIncludeDir()
{
   // FIXME
   return "/usr/local/include";
}

std::string_view getDynamicLibraryExtension()
{
#if defined(_WIN32)
   return "dll";
#elif defined(__APPLE__)
   return "dylib";
#else
   return "so";
#endif
}

void appendToPath(std::vector<char> &Path, std::string_view Append)
{
   if (Path.empty() || Path.back() != PathSeperator)
      Path.push_back(PathSeperator);

   Path.append(Append.begin(), Append.end());
}

void appendToPath(std::vector<char> &Path, const std::string &Append)
{
   appendToPath(Path, std::string_view(Append.str()));
}

void appendToPath(std::string &Path, const std::string &Append)
{
   if (Path.empty() || Path.back() != PathSeperator)
      Path.push_back(PathSeperator);

   std::string append = Append.str();
   Path.insert(Path.end(), append.begin(), append.end());
}

std::string exec(const std::string &cmd)
{
   std::array<char, 128> buffer{};
   std::string result;

   std::shared_ptr<FILE> pipe(popen(cmd.c_str(), "r"), pclose);
   if (!pipe)
      return "";

   while (!feof(pipe.get())) {
      if (fgets(buffer.data(), 128, pipe.get()) != nullptr)
         result += buffer.data();
   }

   return result;
}

} // namespace fs
} // namespace tblgen