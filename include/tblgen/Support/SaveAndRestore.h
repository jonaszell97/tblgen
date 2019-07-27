//
// Created by Jonas Zell on 02.08.18.
//

#ifndef TBLGEN_SAVEANDRESTORE_H
#define TBLGEN_SAVEANDRESTORE_H

#include <llvm/Support/SaveAndRestore.h>

namespace tblgen {
namespace support {

template<class T>
llvm::SaveAndRestore<T> saveAndRestore(T &X)
{
   return llvm::SaveAndRestore<T>(X);
}

template<class T>
llvm::SaveAndRestore<T> saveAndRestore(T &X, const T &NewValue)
{
   return llvm::SaveAndRestore<T>(X, NewValue);
}

} // namespace support
} // namespace tblgen

#endif //TBLGEN_SAVEANDRESTORE_H
