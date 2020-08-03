/*
 * FNV.hpp
 *
 * This header file defines all the hashings related to the
 * FNV-1a used in ReENGINE for hashing strings and/or objects.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "String/Character.hpp"

#define FNV_OFFSET_BASIS 2166136261
#define FNV_PRIME 16777619

namespace Re 
{
	namespace Core 
	{
		namespace Hash 
		{
			/* 
			 * FNV Function
			 *
			 * This function is responsible for hashing any kind of object
			 * using the FNV-1a 32-bit hash, provided the length of the object
			 * is known, in bytes.
			 *
			 * const void* pMem: the pointer to the object to hash.
			 * u32 length: the length, in bytes, of the object to hash.
			 *
			 * return: an unsigned integer representing the object. 
			 *
			 */
			u32 FNV(const void* pMem, u32 length);

			/* 
			 * FNV Function
			 *
			 * This function is responsible for hashing Strings using the
			 * FNV-1a 32-bit hash.
			 *
			 * const utf8* str: the pointer to the string to hash.
			 *
			 * return: an unsigned integer representing the string. 
			 *
			 */
			u32 FNV(const utf8* str);
		}
	}
}
