/*
 * Allocator.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Memory.hpp"

namespace Re 
{
	namespace Memory 
	{
        /**
		 * @brief
		 *
		 */
		struct IAllocator
		{
			virtual ~IAllocator() = default;

            /**
			 * @brief
			 *
			 * @param Size
			 *
			 * @return
			 *
			 */
			virtual void* Allocate(usize Size) = 0;

            /**
			 * @brief
			 *
			 * @param Size 
			 * @param Alignment
			 *
			 * @return
			 *
			 */
			virtual void* AllocateAligned(usize Size, usize Alignment) = 0;

            /**
			 * @brief
			 *
			 * @param Address
			 *
			 */
			virtual void Free(void* Address) = 0;

            /**
			 * @brief
			 *
			 * @param Address
			 *
			 */
			virtual void FreeAligned(void* Address) = 0;
		};
	}
}
