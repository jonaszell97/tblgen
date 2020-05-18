
#include "tblgen/Type.h"

#include "tblgen/TableGen.h"
#include "tblgen/Record.h"
#include "tblgen/Support/Casting.h"

#include <sstream>

using namespace tblgen::support;

namespace tblgen {

std::string Type::toString() const
{
   std::ostringstream ss;
   ss << this;

   return ss.str();
}

std::ostream &operator<<(std::ostream &str, Type const* Ty)
{
   switch (Ty->getTypeID()) {
   case Type::IntTypeID: {
      auto I = cast<IntType>(Ty);
      str << std::string_view(I->isUnsigned() ? "u" : "i")
          << I->getBitWidth();
      break;
   }
   case Type::FloatTypeID:
      str << "f32";
      break;
   case Type::DoubleTypeID:
      str << "f64";
      break;
   case Type::StringTypeID:
      str << "string";
      break;
   case Type::ListTypeID:
      str << "list<";
      str << cast<ListType>(Ty)->getElementType();
      str << ">";
      break;
   case Type::DictTypeID:
      str << "dict<";
      str << cast<DictType>(Ty)->getElementType();
      str << ">";
      break;
   case Type::ClassTypeID:
      str << cast<ClassType>(Ty)->getClass()->getName();
      break;
   case Type::RecordTypeID: {
      auto name = cast<RecordType>(Ty)->getRecord()->getName();
      if (name.empty()) {
         str << "<anonymous '" << cast<RecordType>(Ty)->getRecord()->getBases().front().getBase()->getName() << "'>";
      }
      else {
         str << name;
      }

      break;
   }
   case Type::EnumTypeID:
      str << cast<EnumType>(Ty)->getEnum()->getName();
      break;
   case Type::CodeTypeID:
      str << "code";
      break;
   default:
      unreachable("unhandled type");
   }

   return str;
}

} // namespace tblgen