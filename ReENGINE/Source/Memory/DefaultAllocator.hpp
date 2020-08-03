/*
 * DefaultAllocator.hpp
 *
 * This header file declares the Global Default Allocator for the Newton
 * Engine, which acts as an interface between the Engine and the Operating System.
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
		/*
		 * @brief This class is responsible for handling normal allocations that ask the
		 * operating system or the HeapAllocator for memory.
		 *
		 */
		class NDefaultAllocator : public IAllocator
		{
		public:
			/*
			 * @brief This static method allocates memory from the Operating System and
			 * returns it as a void pointer.
			 *
			 * @param Size: an unsigned integer with the size of the chunk to allocate.
			 * @return a void pointer to a chunk of memory.
			 *
			 */
			virtual void* Allocate(usize Size) override;

			/*
			 * @brief This static method allocates aligned memory from the Operating System and
			 * returns it as a void pointer.
			 *
			 * @param Size: an unsigned integer with the size of the chunk to allocate.
			 * @param Alignment: an unsigned integer with the alignment of the chunk to allocate.
			 * @return a void pointer to a chunk of memory.
			 *
			 */
			virtual void* AllocateAligned(usize Size, usize Alignment) override;

			/*
			 * @brief This static method frees previously allocated memory
			 * with the Operating System.
			 *
			 * @param Address: the pointer to the chunk of memory to free.
			 *
			 */
			virtual void Free(void* Address) override;

			/*
			 * @brief This static method frees previously allocated aligned memory
			 * with the Operating System.
			 *
			 * @param Address: the pointer to the aligned chunk of memory to free.
			 *
			 */
			virtual void FreeAligned(void* Address) override;
		};
	}
}
