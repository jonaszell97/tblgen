
#include "tblgen/Support/DynamicLibrary.h"

#ifdef _WIN32
#   define OS_IS_WINDOWS
#   include <windows.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
#   include <dlfcn.h>
#else
#   error "unsupported operating system!"
#endif

using namespace tblgen;
using namespace tblgen::support;

DynamicLibrary::DynamicLibrary(void *dylib) : dylib(dylib) {}

DynamicLibrary::DynamicLibrary(DynamicLibrary &&other) : dylib(other.dylib)
{
   other.dylib = nullptr;
}

DynamicLibrary &DynamicLibrary::operator=(DynamicLibrary &&other)
{
   dylib = other.dylib;
   other.dylib = nullptr;

   return *this;
}

DynamicLibrary::~DynamicLibrary()
{
#ifdef OS_IS_WINDOWS
   FreeLibrary(dylib);
#else
   dlclose(dylib);
#endif
}

DynamicLibrary DynamicLibrary::Open(const std::string &pathToLib,
                                    std::string *errMsg)
{
#ifdef OS_IS_WINDOWS
   void *dylib = LoadLibrary(pathToLib.c_str());
   if (!dylib) {
      if (errMsg) {
         *errMsg = "could not load dynamic library";
      }
   }
#else
   void *dylib = dlopen(pathToLib.c_str(), RTLD_LAZY);
   if (!dylib) {
      if (errMsg) {
         *errMsg = dlerror();
      }
   }
#endif

   return DynamicLibrary(dylib);
}

void *DynamicLibrary::getAddressOfSymbol(const std::string &symbol) const
{
#ifdef OS_IS_WINDOWS
   return GetProcAddress(dylib, symbol.c_str());
#else
   return dlsym(dylib, symbol.c_str());
#endif
}