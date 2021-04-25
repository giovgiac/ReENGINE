/*
 * Char.cpp
 *
 * This source file defines the functions related to the NCHAR UTF-8 character declared
 * in the Char.hpp header file.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Character.hpp"

/*
 * This macro checks whether the beginning of a character is 10, which indicates
 * the presence of a multi-byte character.
 *
 */
#define IsUTF(x) (((x)&0xC0) != 0x80)

namespace Re
{
	namespace Core
	{
		utf32 NCharNext(const utf8* str, i32* index) {
			// Create Variables
			utf32 ch	= 0;
			i32 size	= 0;

			do {
				ch <<= 6;
				ch += (u8)str[(*index)++];
				size++;
			} while (str[*index] && !IsUTF(str[*index]));
			ch -= UTF8Offsets[size - 1];

			return ch;
		}

		i32 NStrBytes(const utf8* str) {
			// Create Variables
			i32 bytes	= 1;
			i32 i		= 0;

			while (str[i++] != NTEXT('\0'))
				bytes++;

			return bytes;
		}

		bool NStrCmp(const utf8* left, const utf8* right) {
			// Get String IDs
			i32 leftBytes = NStrBytes(left);
			i32 rightBytes = NStrBytes(right);

			if (leftBytes == rightBytes)
				return Re::Memory::Compare(left, right, leftBytes);
			return false;
		}

		i32 NStrLen(const utf8* str) {
			// Create Variables
			i32 len = 0;
			i32 i = 0;

			while (NCharNext(str, &i) != 0)
				len++;

			return len;
		}
	}
}
