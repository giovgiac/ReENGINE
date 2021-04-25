/*
 * Memory.hpp
 * 
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

namespace Re 
{
	namespace Memory 
	{
		/*
		 * @brief This function compares whether two memory regions are equal, given their
		 * size and pointers to their initial addressess.
		 *
		 * @param const void* left: the pointer to the left-hand side of the comparison.
		 * @param const void* right: the pointer to the right-hand side of the comparison.
		 * @param u32 size: the size of the memory region to cover.
		 *
		 * @return: nothing.
		 *
		 */
		extern bool MEMCALL Compare(const void* left, const void* right, u32 size);

		/*
		 * @brief This function copies the contents of a region in memory to another region
		 * in memory, given pointers to those regions and their size.
		 *
		 * @param void* destination: the pointer to the destination location.
		 * @param const void* source: the pointer to the source location.
		 * @param u32 size: the amount of memory to copy.
		 *
		 * @return: nothing.
		 *
		 */
		extern void MEMCALL Copy(void* destination, const void* source, usize size);

		/*
		 * @brief This function moves contents from a region in memory to another region
		 * in memory, given pointers to the regions and the size.
		 *
		 * @param void* destination: the pointer to the destination location.
		 * @param void* source: the pointer to the source location.
		 * @param u32 size: the amount of memory to move.
		 *
		 * @return: nothing.
		 *
		 */
		extern void MEMCALL Move(void* destination, void* source, usize size);

		/*
		 * @brief This function sets the contents from a region in memory to a specified
		 * value.
		 *
		 * @param void* destination: the pointer to the destination location.
		 * @param u8 value: the value to fill the region with.
		 * @param u32 size: the amount of memory to set.
		 *
		 * @return: nothing.
		 *
		 */
		extern void MEMCALL Set(void* destination, u8 value, u32 size);
	}
}
