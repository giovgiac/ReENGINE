/*
 * Allocator.hpp
 *
 * ...
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
		struct IAllocator
		{
			virtual void* Allocate(usize Size) = 0;
			virtual void* AllocateAligned(usize Size, usize Alignment) = 0;
			virtual void Free(void* Address) = 0;
			virtual void FreeAligned(void* Address) = 0;
		};
	}
}
