//
// Created by Jonas Zell on 13.10.17.
//

#ifndef TBLGEN_FILEUTILS_H
#define TBLGEN_FILEUTILS_H

#include <string>
#include <vector>
#include <system_error>
#include <llvm/ADT/ArrayRef.h>

namespace llvm {
   class raw_fd_ostream;
   class MemoryBuffer;
} // namespace llvm

namespace tblgen {
namespace fs {

#ifdef _WIN32
   static char PATH_SEPARATOR = '\\';
#else
   static char PathSeperator = '/';
#endif

std::string_view getPath(std::string_view fullPath);
std::string_view getFileName(std::string_view fullPath);

std::string_view getExtension(std::string_view fullPath);
std::string_view withoutExtension(std::string_view fullPath);
std::string swapExtension(std::string_view fileName,
                          std::string_view newExt);

int deleteFile(std::string_view FileName);

std::string_view getFileNameAndExtension(std::string_view fullPath);
bool fileExists(std::string_view name);

void createDirectories(std::string_view fullPath);
int deleteDirectory(const std::string& Dir);

std::vector<std::string> getAllFilesInDirectory(std::string_view dirName,
                                                bool recursive = false);

void deleteAllFilesInDirectory(const std::string &Dir);

std::string findFileInDirectories(std::string_view fileName,
                                  const std::vector<std::string> &directories);

int executeCommand(std::string_view Program, const std::vector<std::string> &args);
long long getLastModifiedTime(std::string const& pathToFile);

void getAllMatchingFiles(std::string_view fileName,
                         std::vector<std::string> &Out);

std::error_code makeAbsolute(std::vector<char> &Buf);

std::string_view getLibraryDir();
std::string_view getIncludeDir();
std::string getApplicationDir();

std::string_view getDynamicLibraryExtension();

void appendToPath(std::vector<char> &Path, std::string_view Append);
void appendToPath(std::vector<char> &Path, const std::string &Append);
void appendToPath(std::string &Path, const std::string &Append);

std::string getTmpFileName(std::string_view Ext);
std::string exec(const std::string &cmd);

} // namespace fs
} // namespace tblgen


#endif //TBLGEN_FILEUTILS_H
