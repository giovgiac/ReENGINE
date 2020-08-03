/*
 * HAL.hpp
 *
 * This header file is the ReENGINE's Platform Detection subsystem.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

/* Platform Settings */
#ifndef PLATFORM_WINDOWS
#define PLATFORM_WINDOWS 1
#endif

#ifndef PLATFORM_LINUX
#define PLATFORM_LINUX 0
#endif

/* Specific Platform Settings */
#if PLATFORM_WINDOWS
#include "Win32/Platform.hpp"
#elif PLATFORM_LINUX
// Include Linux Settings
#error "Unknown Platform..."
#endif

/* Default Platform Settings */
#ifndef ALIGNOF
#define ALIGNOF
#endif

#ifndef FASTCALL
#define FASTCALL
#endif

#ifndef FORCEINLINE
#define FORCEINLINE
#endif

#ifndef INLINE
#define INLINE
#endif

#ifndef MEMCALL
#define MEMCALL
#endif

#ifndef NTEXT
#define NTEXT(x) x
#endif

#ifndef STDCALL
#define STDCALL
#endif

#ifndef VECTORCALL
#define VECTORCALL
#endif

#ifndef PLATFORM_USE_VULKAN
#define PLATFORM_USE_VULKAN 1
#endif

/* Platform Global Types */
typedef Re::u8			u8;
typedef Re::u16			u16;
typedef Re::u32			u32;
typedef Re::u64			u64;
typedef Re::usize		usize;
typedef Re::i8			i8;
typedef Re::i16			i16;
typedef Re::i32			i32;
typedef Re::i64			i64;
typedef Re::isize		isize;
typedef Re::f32			f32;
typedef Re::f64			f64;
typedef Re::utf8		utf8;
typedef Re::utf32		utf32;
typedef Re::NRESULT		NRESULT;

/* Assert Type Size */
static_assert(sizeof(u8) == 1, "Invalid byte size for u8.");
static_assert(sizeof(u16) == 2, "Invalid byte size for u16.");
static_assert(sizeof(u32) == 4, "Invalid byte size for u32.");
static_assert(sizeof(u64) == 8, "Invalid byte size for u64.");
static_assert(sizeof(i16) == 2, "Invalid byte size for i16.");
static_assert(sizeof(i32) == 4, "Invalid byte size for i32.");
static_assert(sizeof(i64) == 8, "Invalid byte size for i64.");
static_assert(sizeof(f32) == 4, "Invalid byte size for f32.");
static_assert(sizeof(f64) == 8, "Invalid byte size for f64.");
static_assert(sizeof(utf8) == 1, "Invalid byte size for utf8.");
static_assert(sizeof(utf32) == 4, "Invalid byte size for utf32.");
