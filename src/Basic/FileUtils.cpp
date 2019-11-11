
#include "tblgen/Basic/FileUtils.h"

#include <cstdio>
#include <cstdlib>
#include <filesystem>

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

bool fileExists(const std::string &name)
{
   if (FILE *file = fopen(name.c_str(), "r")) {
      fclose(file);
      return true;
   }
   else {
      return false;
   }
}


bool fileExists(std::string_view name)
{
   return fileExists(string(name));
}

string findFileInDirectories(std::string_view fileName,
                             const std::vector<std::string> &directories) {
   if (fileName.front() == fs::PathSeperator) {
      if (fileExists(fileName))
         return std::string(fileName);

      return "";
   }

   auto Path = fs::getPath(fileName);
   if (!Path.empty()) {
      fileName = fs::getFileNameAndExtension(fileName);
   }

   for (std::string dirName : directories) {
      if (!Path.empty()) {
         if (dirName.back() != fs::PathSeperator) {
            dirName += fs::PathSeperator;
         }

         dirName += Path;
      }

      for (auto &entry : std::filesystem::directory_iterator(dirName)) {
         if (entry.is_regular_file() || entry.is_symlink()) {
            string path = entry.path().u8string();
            if (getFileNameAndExtension(path) == fileName) {
               return path;
            }
         }
      }
   }

   return "";
}

} // namespace fs
} // namespace tblgen