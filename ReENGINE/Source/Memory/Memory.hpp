/*
 * Memory.hpp
 *
 * This header file declares functions and classes that are related to the
 * Memory cross-platform subsystem employed by the ReENGINE.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

namespace Re {
	namespace Memory {
		/*
		 * NMemCmp Function
		 *
		 * This function compares whether two memory regions are equal, given their
		 * size and pointers to their initial addressess.
		 *
		 * const void* left: the pointer to the left-hand side of the comparison.
		 * const void* right: the pointer to the right-hand side of the comparison.
		 * u32 size: the size of the memory region to cover.
		 *
		 * return: nothing.
		 *
		 */
		extern bool MEMCALL NMemCmp(const void* left, const void* right, u32 size);

		/*
		 * NMemCpy Function
		 *
		 * This function copies the contents of a region in memory to another region
		 * in memory, given pointers to those regions and their size.
		 *
		 * void* destination: the pointer to the destination location.
		 * const void* source: the pointer to the source location.
		 * u32 size: the amount of memory to copy.
		 *
		 * return: nothing.
		 *
		 */
		extern void MEMCALL NMemCpy(void* destination, const void* source, u32 size);

		/*
		 * NMemMove Function
		 *
		 * This function moves contents from a region in memory to another region
		 * in memory, given pointers to the regions and the size.
		 *
		 * void* destination: the pointer to the destination location.
		 * void* source: the pointer to the source location.
		 * u32 size: the amount of memory to move.
		 *
		 * return: nothing.
		 *
		 */
		extern void MEMCALL NMemMove(void* destination, void* source, u32 size);

		/*
		 * NMemSet Function
		 *
		 * This function sets the contents from a region in memory to a specified
		 * value.
		 *
		 * void* destination: the pointer to the destination location.
		 * u8 value: the value to fill the region with.
		 * u32 size: the amount of memory to set.
		 *
		 * return: nothing.
		 *
		 */
		extern void MEMCALL NMemSet(void* destination, u8 value, u32 size);
	}
}
