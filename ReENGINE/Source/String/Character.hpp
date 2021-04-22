/*
 * Character.hpp
 *
 * This header file declares the functions related to the characters used in the ReENGINE which
 * are based on the multi-byte encoding known as UTF-8.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Memory/DefaultAllocator.hpp"

namespace Re
{
	namespace Core
	{
		/**
		 * @brief This array contains a couple of offsets used in the reading of UTF-8
		 * characters.
		 *
		 */
		static const utf32 UTF8Offsets[] = {
			0x00000000UL, 0x00003080UL, 0x000E2080UL,
			0x03C82080UL, 0xFA082080UL, 0x82082080UL
		};

		/**
		 * @brief This function advances the character by one in an UTF-8 string and updates
		 * an index passed in.
		 *
		 * @param str: the string to advance a character.
		 * @param index: the pointer to the index to update.
		 *
		 * @return the character advanced to.
		 *
		 */
		extern utf32 NCharNext(const utf8* str, i32* index);

		/**
		 * @brief This function calculates and returns the amount of bytes occupied by an
		 * UTF-8 string.
		 *
		 * @param str: the string to calculate the number of bytes.
		 *
		 * @return the number of bytes used by the string.
		 *
		 */
		extern i32 NStrBytes(const utf8* str);

		/**
		 * @brief This function compares two strings of characters of UTF-8 encoding.
		 *
		 * @return true if the strings are equal, false otherwise.
		 *
		 */
		extern bool NStrCmp(const utf8* left, const utf8* right);

		/**
		 * @brief This function duplicates a string of characters of UTF-8 encoding.
		 * It allocates space in memory using the NDefaultAllocator, unless
		 * specified otherwise.
		 *
		 * @param str: the UTF-8 string to duplicate.
		 *
		 * @return a pointer to the duplicated version of the string.
		 *
		 */
		extern INLINE utf8* NStrDup(const utf8* str)
		{
			Memory::NDefaultAllocator alloc;
			utf8* result = nullptr;
			i32 size = NStrBytes(str);

			// Allocate and Copy String
			result = static_cast<utf8*>(alloc.AllocateAligned(sizeof(utf8) * size, ALIGNOF(utf8)));
			Re::Memory::NMemCpy(result, str, sizeof(utf8) * size);
			return result;
		}

		/**
		 * @brief This function calculates the length of one string made of UTF-8 characters
		 * and returns it.
		 *
		 * @param str: the UTF-8 string to calculate the length of.
		 *
		 * @return an unsigned integer with the length of the string.
		 *
		 */
		extern i32 NStrLen(const utf8* str);
	}
}
