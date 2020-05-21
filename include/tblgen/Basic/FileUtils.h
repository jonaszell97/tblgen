
#ifndef TBLGEN_FILEUTILS_H
#define TBLGEN_FILEUTILS_H

#include <string>
#include <vector>
#include <system_error>

namespace tblgen {
namespace fs {

std::string_view getPath(std::string_view fullPath);
std::string_view getFileName(std::string_view fullPath);
std::string_view getExtension(std::string_view fullPath);

std::string_view getFileNameAndExtension(std::string_view fullPath);
bool fileExists(std::string_view name);
bool fileExists(const std::string &name);

std::string findFileInDirectories(std::string_view fileName,
                                  const std::vector<std::string> &directories);


} // namespace fs
} // namespace tblgen


#endif //TBLGEN_FILEUTILS_H
