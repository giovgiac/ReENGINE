/*
 * Memory.cpp
 *
 * This source file defines the functions and methods relative to the Memory subsystem
 * described in the Memory.hpp header file.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Memory.hpp"

#define lmask (sizeof(long) - 1)

namespace Re {
	namespace Memory {
		bool MEMCALL NMemCmp(const void* left, const void* right, u32 size) {
			bool res = true;

			// Read Memory as Bytes
			const u8* l = reinterpret_cast<const u8*>(left);
			const u8* r = reinterpret_cast<const u8*>(right);

			for (u32 i = 0; i < size; i++)
				res = l[i] == r[i] ? res : false;

			return res;
		}

		void MEMCALL NMemCpy(void* destination, const void* source, usize size) {
			const u8* s = static_cast<const u8*>(source);
			u8* d = static_cast<u8*>(destination);
			i32 len;

			if (size == 0 || destination == source)
				return;

			if (((long)d | (long)s) & lmask) {
				if ((((long)d ^ (long)s) & lmask) || (size < sizeof(long)))
					len = size;
				else
					len = sizeof(long) - ((long)d & lmask);

				size -= len;
				for (; len > 0; len--)
					*d++ = *s++;
			}

			for (len = size / sizeof(long); len > 0; len--) {
				*(long*)d = *(long*)s;
				d += sizeof(long);
				s += sizeof(long);
			}

			for (len = size & lmask; len > 0; len--)
				*d++ = *s++;
				
			// Read Memory as Bytes
			/*const u8* src = reinterpret_cast<const u8*>(source);
			u8* dst = reinterpret_cast<u8*>(destination);

			for (u32 i = 0; i < size; i++)
				dst[i] = src[i];
			*/
		}

		void MEMCALL NMemMove(void* destination, void* source, usize size) {
			// Read Memory as Bytes
			u8* src = static_cast<u8*>(source);
			u8* dst = static_cast<u8*>(destination);

			if (src == dst) {
				// If Already Equal
				return;
			} else if (src < dst) {
				// Copy From Back
				for (src = src + size - 1, dst = dst + size - 1; size > 0; size--)
					*dst-- = *src--;
			} else {
				// Copy From Front
				for (; size > 0; size--)
					*dst++ = *src++;
			}
		}

		void MEMCALL NMemSet(void* destination, u8 value, u32 size) {
			// Read Memory as Bytes
			u8* dst = reinterpret_cast<u8*>(destination);

			for (u32 i = 0; i < size; i++)
				dst[i] = value;
		}
	}
}
