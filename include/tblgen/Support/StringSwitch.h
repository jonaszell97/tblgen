
#ifndef TBLGEN_STRINGSWITCH_H
#define TBLGEN_STRINGSWITCH_H

#include "tblgen/Support/Optional.h"

#include <cassert>
#include <cstring>

namespace tblgen {

template<class T>
class StringSwitch {
private:
   std::string strToMatch;
   support::Optional<T> result;

public:
   explicit StringSwitch(std::string_view strToMatch) : strToMatch(strToMatch)
   {}

   explicit StringSwitch(std::string &&strToMatch) : strToMatch(move(strToMatch))
   {}

   StringSwitch(const StringSwitch &) = delete;

   void operator=(const StringSwitch &) = delete;
   void operator=(StringSwitch &&other) = delete;

   StringSwitch &Case(std::string_view match, const T& value)
   {
      if (!result.hasValue() && match == strToMatch)
      {
         result = value;
      }

      return *this;
   }

   T Default(const T& defaultValue)
   {
      if (result.hasValue())
      {
         return result.getValue();
      }

      return defaultValue;
   }

   [[nodiscard]] operator T()
   {
      assert(result.hasValue() && "StringSwitch does not have a value!");
      return std::move(result.getValue());
   }
};

} // namespace tblgen

#endif //TBLGEN_STRINGSWITCH_H
