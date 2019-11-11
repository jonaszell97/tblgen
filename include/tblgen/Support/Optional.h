
#ifndef TABLEGEN_OPTIONAL_H
#define TABLEGEN_OPTIONAL_H

#include <cstddef>

namespace tblgen::support
{

struct NoneType { };
inline NoneType None;

template<class T>
struct Optional {
private:
   alignas(T) std::byte storage[sizeof(T)];
   bool isSet;

   void destroyIfPresent()
   {
      if (!isSet) {
         return;
      }

      reinterpret_cast<T *>(&storage)->~T();
   }

public:
   /*implicit*/ Optional(NoneType _ = None)
      : isSet(false)
   { }

   /*implicit*/ Optional(const T &value)
      : isSet(true)
   {
      new(&storage) T(value);
   }

   /*implicit*/ Optional(T &&value)
      : isSet(true)
   {
      new(&storage) T(std::move(value));
   }

   Optional &operator=(NoneType _)
   {
      destroyIfPresent();
      isSet = false;

      return *this;
   }

   Optional &operator=(const T &value)
   {
      destroyIfPresent();

      new(&storage) T(value);
      isSet = true;

      return *this;
   }

   Optional &operator=(T &&value) noexcept
   {
      destroyIfPresent();

      new(&storage) T(std::move(value));
      isSet = true;

      return *this;
   }

   bool hasValue() const { return isSet; }
   /*implicit*/ operator bool() const { return isSet; }

   const T& getValue() const
   {
      assert(hasValue() && "Optional has no value!");
      return *reinterpret_cast<const T*>(&storage);
   }

   T& getValue()
   {
      assert(hasValue() && "Optional has no value!");
      return *reinterpret_cast<T*>(&storage);
   }
};

} // namespace tblgen::support

#endif //TABLEGEN_OPTIONAL_H
