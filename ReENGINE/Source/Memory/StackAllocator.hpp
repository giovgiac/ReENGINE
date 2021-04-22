/*
 * StackAllocator.hpp
 *
 * This header file declares the NStackAllocator class which
 * has an important role in substituting direct heap allocation throughout
 * the ReENGINE.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Allocator.hpp"

namespace Re
{
    namespace Memory
    {
        /**
         * @brief This class is responsible for managing and handling
         * memory that is held on Stack-based allocators.
         *
         */
        template <usize StackSize>
        class NStackAllocator : public IAllocator
        {
        private:
            u8* Marker;
            usize Offset;

        public:
            /**
             * @brief This constructor allocates a block of memory of InStackSize, in
             * bytes, and sets the offset to 0.
             *
             */
            NStackAllocator()
                : Offset(0)
            {
                Marker = new u8[StackSize];
            }

            /**
             * @brief This destructor frees the block of memory previously allocated.
             *
             */
            ~NStackAllocator()
            {
                delete[] Marker;
            }

            /**
             * @brief This method allocates an array in the stack of specified size,
             * in bytes, and increases the offset by that same size.
             *
             * @param Size: the size of the chunk to allocate.
             *
             * @return a pointer to a block of memory of size (in bytes).
             *
             */
            INLINE virtual void* Allocate(usize Size) override
            {
                ASSERT(Size <= StackSize);
                ASSERT(Size + Offset <= StackSize);

                void* Result = Marker + Offset;
                Offset += Size;
                return Result;
            }

            /**
             * @brief This method allocates an array in the stack of specified
             * size and alignment, both in bytes, and calculates the
             * appropriate address.
             *
             * @param Size: the size of the chunk to allocate.
             * @param Alignment: the alignment of the chunk to allocate.
             *
             * @return a pointer to a aligned block of memory of size and aligment (in bytes).
             *
             */
            INLINE virtual void* AllocateAligned(usize Size, usize Alignment) override
            {
                ASSERT(Alignment >= 1);
                ASSERT(Alignment <= 128);
                ASSERT((Alignment & (Alignment - 1)) == 0);

                // Get Total Memory to Allocate
                u32 expandedSize = Size + Alignment;

                // Allocate Total Unaligned Memory
                u32 rawAddress = reinterpret_cast<u32>(Allocate(expandedSize));

                // Calculate the Adjustment
                u32 mask = (Alignment - 1);
                u32 misalignment = (rawAddress & mask);
                i32 adjustment = Alignment - misalignment;

                // Calculate Adjusted Address
                u32 alignedAddress = rawAddress + adjustment;

                // Store the Adjustment
                ASSERT(adjustment < 256);
                u8* pAligned = reinterpret_cast<u8*>(alignedAddress);
                pAligned[-1] = static_cast<u8>(adjustment);

                return static_cast<void*>(pAligned);
            }

            /**
             * @brief This method clears the stack by setting the offset back to 0,
             * and making all memory available again.
             *
             */
            INLINE void Clear()
            {
                Offset = 0;
            }

            /**
             * @brief This method frees an specified unaligned memory address of the stack,
             * making it's memory available once again.
             *
             * @param Address: a pointer to the memory address to be freed.
             *
             */
            INLINE virtual void Free(void* Address) override
            {
                // Read Memory as Address
                u32 address = reinterpret_cast<u32>(Address);
                u32 offset = address - reinterpret_cast<u32>(Marker);

                // Set Offset
                Offset = offset;
            }

            /**
             * @brief This method frees an specific aligned memory address of the stack,
             * making it's memory available once again.
             *
             * @param Address: a pointer to the aligned memory address to be freed.
             *
             */
            INLINE virtual void FreeAligned(void* Address) override
            {
                const u8* pAlignedMem = reinterpret_cast<const u8*>(Address);

                // Read Aligned Address and Get Adjustment
                u32 alignedAddress = reinterpret_cast<u32>(Address);
                i32 adjusment = static_cast<i32>(pAlignedMem[-1]);

                // Trace Raw Address and Obtain Pointer
                u32 rawAddress = alignedAddress - adjusment;
                void* pUnalignedMem = reinterpret_cast<void*>(rawAddress);

                Free(pUnalignedMem);
            }

            /**
             * @brief This method is a getter for the offset attribute, which measures,
             * in bytes, the distance from the origin of the stack.
             *
             * @return a number representing the byte offset from the stack's origin.
             *
             */
            INLINE u32 GetOffset() const { return Offset; }
        };
    }
}
