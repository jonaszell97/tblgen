
#ifndef TABLEGEN_DYNAMICLIBRARY_H
#define TABLEGEN_DYNAMICLIBRARY_H

#include <string>

namespace tblgen::support {

class DynamicLibrary {
   /// The opaque pointer to the dynamic library returned from a syscall.
   void *dylib;

   /// Private c'tor.
   explicit DynamicLibrary(void *dylib);

public:
   /// Open a dynamic library permanently.
   static DynamicLibrary Open(const std::string &pathToLib,
                              std::string *errMsg = nullptr);

   /// D'tor for cleaning up resources.
   ~DynamicLibrary();

   /// Move c'tor.
   explicit DynamicLibrary(DynamicLibrary &&other);

   // Move assignment.
   DynamicLibrary &operator=(DynamicLibrary &&other);

   /// Disallow copy c'tor.
   DynamicLibrary(const DynamicLibrary &) = delete;

   // Disallow copy assignment.
   DynamicLibrary &operator=(const DynamicLibrary &) = delete;

   /// Search for a symbol in the library.
   void *getAddressOfSymbol(const std::string &symbol) const;
};

} // namespace tblgen::support

#endif // TABLEGEN_DYNAMICLIBRARY_H
