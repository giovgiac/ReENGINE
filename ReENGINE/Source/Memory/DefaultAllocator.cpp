/*
 * DefaultAllocator.cpp
 *
 * This source file defines the methods declared in the
 * DefaultAllocator.hpp header file.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "DefaultAllocator.hpp"

namespace Re {
namespace Memory {

void* DefaultAllocator::Allocate(usize Size) 
{
	return malloc(Size);
}

void* DefaultAllocator::AllocateAligned(usize Size, usize Alignment) 
{
	return _aligned_malloc(Size, Alignment);
	/*
	ASSERT(alignment >= 1);
	ASSERT(alignment <= 128);
	ASSERT((alignment & (alignment - 1)) == 0);

	// Get Total Memory to Allocate
	u32 expandedSize = size + alignment;

	// Allocate Total Unaligned Memory
	u32 rawAddress = reinterpret_cast<u32>(Allocate(expandedSize));

	// Calculate the Adjustment
	u32 mask = (alignment - 1);
	u32 misalignment = (rawAddress & mask);
	i32 adjustment = alignment - misalignment;

	// Calculate Adjusted Address
	u32 alignedAddress = rawAddress + adjustment;

	// Store the Adjustment
	ASSERT(adjustment < 256);
	u8* pAligned = reinterpret_cast<u8*>(alignedAddress);
	pAligned[-1] = static_cast<u8>(adjustment);

	return static_cast<void*>(pAligned);
	*/
}

void DefaultAllocator::Free(void* Address) 
{
	free(Address);
}

void DefaultAllocator::FreeAligned(void* Address) 
{
	_aligned_free(Address);
	/*
	const u8* pAlignedMem = reinterpret_cast<const u8*>(pAddress);

	// Read Aligned Address and Get Adjustment
	u32 alignedAddress = reinterpret_cast<u32>(pAddress);
	i32 adjusment = static_cast<i32>(pAlignedMem[-1]);

	// Trace Raw Address and Obtain Pointer
	u32 rawAddress = alignedAddress - adjusment;
	void* pUnalignedMem = reinterpret_cast<void*>(rawAddress);

	Free(pUnalignedMem);
	*/
}

}}
