
#ifndef TBLGEN_SAVEANDRESTORE_H
#define TBLGEN_SAVEANDRESTORE_H

namespace tblgen {
namespace support {

template<class T>
struct SaveAndRestore {
public:
   explicit SaveAndRestore(T &value) : ref(value), oldValue(value)
   {
   }

   SaveAndRestore(T &value, const T &newValue) : ref(value), oldValue(value)
   {
      value = newValue;
   }

   ~SaveAndRestore()
   {
      ref = oldValue;
   }

private:
   T &ref;
   T oldValue;
};

template<class T>
SaveAndRestore<T> saveAndRestore(T &X)
{
   return SaveAndRestore<T>(X);
}

template<class T>
SaveAndRestore<T> saveAndRestore(T &X, const T &NewValue)
{
   return SaveAndRestore<T>(X, NewValue);
}

} // namespace support
} // namespace tblgen

#endif //TBLGEN_SAVEANDRESTORE_H
