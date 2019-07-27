//
// Created by Jonas Zell on 19.11.17.
//

#ifndef TBLGEN_CASTING_H
#define TBLGEN_CASTING_H

#include <llvm/Support/Casting.h>

namespace tblgen {
namespace support {

using llvm::cast;
using llvm::dyn_cast;
using llvm::isa;
using llvm::cast_or_null;
using llvm::dyn_cast_or_null;

} // namespace support
} // namespace tblgen

#endif //TBLGEN_CASTING_H
