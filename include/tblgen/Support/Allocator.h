
#ifndef TABLEGEN_ALLOCATOR_H
#define TABLEGEN_ALLOCATOR_H

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iterator>
#include <type_traits>
#include <utility>
#include <vector>

namespace tblgen::support {

inline void *alignAddr(void *Addr, size_t Alignment)
{
   assert((uintptr_t)Addr + Alignment - 1 >= (uintptr_t)Addr);
   return (void*)((char*)Addr + (((uintptr_t)Addr + Alignment - 1) & ~(uintptr_t)(Alignment - 1)));
}

class ArenaAllocator {
private:
   struct Slab {
      void *beginPtr;
      void *currentPtr;
      size_t size;
   };

   const unsigned SlabSize;
   std::vector<Slab> slabs;
   unsigned totalAllocatedBytes;
   unsigned totalUsedBytes;

   Slab &CreateSlab(size_t size = -1, unsigned alignment = 16)
   {
      if (size == -1) {
         size = SlabSize;
      }
      if ((size % alignment) != 0) {
         size += size % alignment;
      }

      totalAllocatedBytes += size;

      void *mem = malloc(size);
      slabs.push_back(Slab { mem, mem, size, });

      return slabs.back();
   }

public:
   explicit ArenaAllocator(unsigned SlabSize = 4096)
      : SlabSize(SlabSize), totalAllocatedBytes(0), totalUsedBytes(0)
   {

   }

   ~ArenaAllocator()
   {
      for (auto &slab : slabs) {
         free(slab.beginPtr);
      }
   }

   void *Allocate(size_t size, unsigned alignment)
   {
      // If size is bigger than the slab size, create a custom slab.
      if (size > SlabSize) {
         auto &slab = CreateSlab(size, alignment);
         totalUsedBytes += slab.size;
         slab.currentPtr = (void*)((char*)slab.beginPtr + slab.size);

         return slab.beginPtr;
      }

      // Look for a slab with enough leftover space.
      for (auto &slab : slabs) {
         ptrdiff_t spaceLeft = ((char*)slab.beginPtr + slab.size) - (char*)slab.currentPtr;
         if (spaceLeft > size) {
            // Align address and check if we still have enough space.
            void *alignedPtr = alignAddr(slab.currentPtr, alignment);
            spaceLeft = ((char*)slab.beginPtr + slab.size) - (char*)alignedPtr;

            if (spaceLeft > size) {
               totalUsedBytes += ((char*)alignedPtr - (char*)slab.currentPtr);
               slab.currentPtr = alignedPtr;

               return alignedPtr;
            }
         }
      }

      // Start a new slab.
      auto &slab = CreateSlab(size, alignment);
      totalUsedBytes += size;
      slab.currentPtr = (void*)((char*)slab.beginPtr + size);

      return slab.beginPtr;
   }

   void Deallocate(void *ptr, size_t size)
   {

   }

   template<class T> T *Allocate(size_t Num = 1)
   {
      return static_cast<T *>(Allocate(Num * sizeof(T), alignof(T)));
   }

   template<typename T>
   typename std::enable_if<
      !std::is_same<typename std::remove_cv<T>::type, void>::value, void>::type
   Deallocate(T *Ptr, size_t Num = 1)
   {
      Deallocate(static_cast<void *>(Ptr), Num * sizeof(T));
   }
};

} // namespace tblgen::support

inline void *operator new(size_t size,
                          ::tblgen::support::ArenaAllocator& A,
                          size_t alignment = 8) {
   return A.Allocate(size, alignment);
}

inline void operator delete(void *ptr, ::tblgen::support::ArenaAllocator& A,
                            size_t size) {
   return A.Deallocate(ptr, size);
}

inline void *operator new[](size_t size, ::tblgen::support::ArenaAllocator& A,
                            size_t alignment = 8) {
   return A.Allocate(size, alignment);
}

inline void operator delete[](void *ptr, ::tblgen::support::ArenaAllocator& A,
                              size_t size) {
   return A.Deallocate(ptr, size);
}

#endif // TABLEGEN_ALLOCATOR_H
