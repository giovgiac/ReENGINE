/* 
 * FNV.cpp
 *
 * This source file defines the functions declared in the FNV.hpp
 * header file.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "FNV.hpp"

namespace Re 
{
	namespace Core 
	{
		namespace Hash 
		{
			u32 FNV(const void* pMem, u32 length) {
				u32 result = FNV_OFFSET_BASIS;
				const u8* pBytes = reinterpret_cast<const u8*>(pMem);

				for (u32 i = 0; i < length; i++) {
					result ^= pBytes[i];
					result *= FNV_PRIME;
				}

				return result;
			}

			u32 FNV(const utf8* str) {
				i32 i = 0;
				u32 result = FNV_OFFSET_BASIS;

				while (NCharNext(str, &i) != 0) {
					result ^= str[i];
					result *= FNV_PRIME;
				}

				return result;
			}
		}
	}
}
