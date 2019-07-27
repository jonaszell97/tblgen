
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

/// \brief Aligns \c Addr to \c Alignment bytes, rounding up.
///
/// Alignment should be a power of two.  This method rounds up, so
/// alignAddr(7, 4) == 8 and alignAddr(8, 4) == 8.
inline uintptr_t alignAddr(const void *Addr, size_t Alignment) {
   assert((uintptr_t)Addr + Alignment - 1 >= (uintptr_t)Addr);
   return (((uintptr_t)Addr + Alignment - 1) & ~(uintptr_t)(Alignment - 1));
}

/// \brief CRTP base class providing obvious overloads for the core \c
/// Allocate() methods of LLVM-style allocators.
///
/// This base class both documents the full public interface exposed by all
/// LLVM-style allocators, and redirects all of the overloads to a single core
/// set of methods which the derived class must define.
template<typename DerivedT> class AllocatorBase {
public:
   /// \brief Allocate \a Size bytes of \a Alignment aligned memory. This method
   /// must be implemented by \c DerivedT.
   void *Allocate(size_t Size, size_t Alignment)
   {
      return static_cast<DerivedT *>(this)->Allocate(Size, Alignment);
   }

   /// \brief Deallocate \a Ptr to \a Size bytes of memory allocated by this
   /// allocator.
   void Deallocate(const void *Ptr, size_t Size)
   {
      return static_cast<DerivedT *>(this)->Deallocate(Ptr, Size);
   }

   // The rest of these methods are helpers that redirect to one of the above
   // core methods.

   /// \brief Allocate space for a sequence of objects without constructing
   /// them.
   template<typename T> T *Allocate(size_t Num = 1)
   {
      return static_cast<T *>(Allocate(Num * sizeof(T), alignof(T)));
   }

   /// \brief Deallocate space for a sequence of objects without constructing
   /// them.
   template<typename T>
   typename std::enable_if<
       !std::is_same<typename std::remove_cv<T>::type, void>::value, void>::type
   Deallocate(T *Ptr, size_t Num = 1)
   {
      Deallocate(static_cast<const void *>(Ptr), Num * sizeof(T));
   }
};

class MallocAllocator : public AllocatorBase<MallocAllocator> {
public:
   void Reset() {}

   void *Allocate(size_t Size, size_t /*Alignment*/)
   {
      return malloc(Size);
   }

   // Pull in base class overloads.
   using AllocatorBase<MallocAllocator>::Allocate;

   void Deallocate(const void *Ptr, size_t /*Size*/)
   {
      free(const_cast<void *>(Ptr));
   }

   // Pull in base class overloads.
   using AllocatorBase<MallocAllocator>::Deallocate;
};

/// \brief Allocate memory in an ever growing pool, as if by bump-pointer.
///
/// This isn't strictly a bump-pointer allocator as it uses backing slabs of
/// memory rather than relying on a boundless contiguous heap. However, it has
/// bump-pointer semantics in that it is a monotonically growing pool of memory
/// where every allocation is found by merely allocating the next N bytes in
/// the slab, or the next N bytes in the next slab.
///
/// Note that this also has a threshold for forcing allocations above a certain
/// size into their own slab.
///
/// The BumpPtrAllocatorImpl template defaults to using a MallocAllocator
/// object, which wraps malloc, to allocate memory, but it can be changed to
/// use a custom allocator.
template<typename AllocatorT = MallocAllocator, size_t SlabSize = 4096,
         size_t SizeThreshold = SlabSize>
class BumpPtrAllocatorImpl
    : public AllocatorBase<
          BumpPtrAllocatorImpl<AllocatorT, SlabSize, SizeThreshold>> {
public:
   static_assert(SizeThreshold <= SlabSize,
                 "The SizeThreshold must be at most the SlabSize to ensure "
                 "that objects larger than a slab go into their own memory "
                 "allocation.");

   BumpPtrAllocatorImpl() = default;

   template<typename T>
   BumpPtrAllocatorImpl(T &&Allocator)
       : Allocator(std::forward<T &&>(Allocator))
   {
   }

   // Manually implement a move constructor as we must clear the old allocator's
   // slabs as a matter of correctness.
   BumpPtrAllocatorImpl(BumpPtrAllocatorImpl &&Old)
       : CurPtr(Old.CurPtr), End(Old.End), Slabs(std::move(Old.Slabs)),
         CustomSizedSlabs(std::move(Old.CustomSizedSlabs)),
         BytesAllocated(Old.BytesAllocated), RedZoneSize(Old.RedZoneSize),
         Allocator(std::move(Old.Allocator))
   {
      Old.CurPtr = Old.End = nullptr;
      Old.BytesAllocated = 0;
      Old.Slabs.clear();
      Old.CustomSizedSlabs.clear();
   }

   ~BumpPtrAllocatorImpl()
   {
      DeallocateSlabs(Slabs.begin(), Slabs.end());
      DeallocateCustomSizedSlabs();
   }

   BumpPtrAllocatorImpl &operator=(BumpPtrAllocatorImpl &&RHS)
   {
      DeallocateSlabs(Slabs.begin(), Slabs.end());
      DeallocateCustomSizedSlabs();

      CurPtr = RHS.CurPtr;
      End = RHS.End;
      BytesAllocated = RHS.BytesAllocated;
      RedZoneSize = RHS.RedZoneSize;
      Slabs = std::move(RHS.Slabs);
      CustomSizedSlabs = std::move(RHS.CustomSizedSlabs);
      Allocator = std::move(RHS.Allocator);

      RHS.CurPtr = RHS.End = nullptr;
      RHS.BytesAllocated = 0;
      RHS.Slabs.clear();
      RHS.CustomSizedSlabs.clear();
      return *this;
   }

   /// \brief Deallocate all but the current slab and reset the current pointer
   /// to the beginning of it, freeing all memory allocated so far.
   void Reset()
   {
      // Deallocate all but the first slab, and deallocate all custom-sized
      // slabs.
      DeallocateCustomSizedSlabs();
      CustomSizedSlabs.clear();

      if (Slabs.empty())
         return;

      // Reset the state.
      BytesAllocated = 0;
      CurPtr = (char *)Slabs.front();
      End = CurPtr + SlabSize;

      __asan_poison_memory_region(*Slabs.begin(), computeSlabSize(0));
      DeallocateSlabs(std::next(Slabs.begin()), Slabs.end());
      Slabs.erase(std::next(Slabs.begin()), Slabs.end());
   }

   /// \brief Allocate space at the specified alignment.
   void *Allocate(size_t Size, size_t Alignment)
   {
      assert(Alignment > 0
             && "0-byte alignnment is not allowed. Use 1 instead.");

      // Keep track of how many bytes we've allocated.
      BytesAllocated += Size;

      size_t Adjustment = alignmentAdjustment(CurPtr, Alignment);
      assert(Adjustment + Size >= Size
             && "Adjustment + Size must not overflow");

      size_t SizeToAllocate = Size;

      // Check if we have enough space.
      if (Adjustment + SizeToAllocate <= size_t(End - CurPtr)) {
         char *AlignedPtr = CurPtr + Adjustment;
         CurPtr = AlignedPtr + SizeToAllocate;
         return AlignedPtr;
      }

      // If Size is really big, allocate a separate slab for it.
      size_t PaddedSize = SizeToAllocate + Alignment - 1;
      if (PaddedSize > SizeThreshold) {
         void *NewSlab = Allocator.Allocate(PaddedSize, 0);
         CustomSizedSlabs.push_back(std::make_pair(NewSlab, PaddedSize));

         uintptr_t AlignedAddr = alignAddr(NewSlab, Alignment);
         assert(AlignedAddr + Size <= (uintptr_t)NewSlab + PaddedSize);
         char *AlignedPtr = (char *)AlignedAddr;
         return AlignedPtr;
      }

      // Otherwise, start a new slab and try again.
      StartNewSlab();
      uintptr_t AlignedAddr = alignAddr(CurPtr, Alignment);
      assert(AlignedAddr + SizeToAllocate <= (uintptr_t)End
             && "Unable to allocate memory!");
      char *AlignedPtr = (char *)AlignedAddr;
      CurPtr = AlignedPtr + SizeToAllocate;
      return AlignedPtr;
   }

   // Pull in base class overloads.
   using AllocatorBase<BumpPtrAllocatorImpl>::Allocate;

   // Bump pointer allocators are expected to never free their storage; and
   // clients expect pointers to remain valid for non-dereferencing uses even
   // after deallocation.
   void Deallocate(const void *, size_t)
   {
   }

   // Pull in base class overloads.
   using AllocatorBase<BumpPtrAllocatorImpl>::Deallocate;

   size_t GetNumSlabs() const { return Slabs.size() + CustomSizedSlabs.size(); }

   size_t getTotalMemory() const
   {
      size_t TotalMemory = 0;
      for (auto I = Slabs.begin(), E = Slabs.end(); I != E; ++I)
         TotalMemory += computeSlabSize(std::distance(Slabs.begin(), I));
      for (auto &PtrAndSize : CustomSizedSlabs)
         TotalMemory += PtrAndSize.second;
      return TotalMemory;
   }

   size_t getBytesAllocated() const { return BytesAllocated; }

   void setRedZoneSize(size_t NewSize) { RedZoneSize = NewSize; }

private:
   /// \brief The current pointer into the current slab.
   ///
   /// This points to the next free byte in the slab.
   char *CurPtr = nullptr;

   /// \brief The end of the current slab.
   char *End = nullptr;

   /// \brief The slabs allocated so far.
   std::vector<void*> Slabs;

   /// \brief Custom-sized slabs allocated for too-large allocation requests.
   std::vector<std::pair<void *, size_t>> CustomSizedSlabs;

   /// \brief How many bytes we've allocated.
   ///
   /// Used so that we can compute how much space was wasted.
   size_t BytesAllocated = 0;

   /// \brief The number of bytes to put between allocations when running under
   /// a sanitizer.
   size_t RedZoneSize = 1;

   /// \brief The allocator instance we use to get slabs of memory.
   AllocatorT Allocator;

   static size_t computeSlabSize(unsigned SlabIdx)
   {
      // Scale the actual allocated slab size based on the number of slabs
      // allocated. Every 128 slabs allocated, we double the allocated size to
      // reduce allocation frequency, but saturate at multiplying the slab size
      // by 2^30.
      return SlabSize * ((size_t)1 << std::min<size_t>(30, SlabIdx / 128));
   }

   /// \brief Allocate a new slab and move the bump pointers over into the new
   /// slab, modifying CurPtr and End.
   void StartNewSlab()
   {
      size_t AllocatedSlabSize = computeSlabSize(Slabs.size());

      void *NewSlab = Allocator.Allocate(AllocatedSlabSize, 0);

      Slabs.push_back(NewSlab);
      CurPtr = (char *)(NewSlab);
      End = ((char *)NewSlab) + AllocatedSlabSize;
   }

   /// \brief Deallocate a sequence of slabs.
   void DeallocateSlabs(std::vector<void*>::iterator I,
                        std::vector<void*>::iterator E)
   {
      for (; I != E; ++I) {
         size_t AllocatedSlabSize
             = computeSlabSize(std::distance(Slabs.begin(), I));
         Allocator.Deallocate(*I, AllocatedSlabSize);
      }
   }

   /// \brief Deallocate all memory for custom sized slabs.
   void DeallocateCustomSizedSlabs()
   {
      for (auto &PtrAndSize : CustomSizedSlabs) {
         void *Ptr = PtrAndSize.first;
         size_t Size = PtrAndSize.second;
         Allocator.Deallocate(Ptr, Size);
      }
   }

   template<typename T> friend class SpecificBumpPtrAllocator;
};

/// \brief The standard BumpPtrAllocator which just uses the default template
/// parameters.
typedef BumpPtrAllocatorImpl<> BumpPtrAllocator;

} // namespace tblgen::support

#endif // TABLEGEN_ALLOCATOR_H
