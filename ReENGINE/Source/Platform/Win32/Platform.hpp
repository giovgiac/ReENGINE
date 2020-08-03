/*
 * Platform.hpp
 *
 * This header file specifies specific macros, types and functions unique to the
 * Windows Platform.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include <cstdio>
#include <tchar.h>
#include <typeinfo>
#include <Windows.h>

/* Windows Specific Settings */

#define ALIGNOF(x) alignof(x)
#define FASTCALL __fastcall
#define FORCEINLINE __forceinline
#define INLINE __inline
#define MEMCALL __cdecl
#define NTEXT(x) u8 ## x
#define STDCALL __stdcall
#define VECTORCALL __vectorcall

#ifdef _DEBUG
#define ASSERTIONS
#endif

/* Windows Specific Macros */

#define NSUCCESS 0
#define NFAILURE 1

/* Windows Specific Atomic Data Types */

namespace Re 
{
	typedef unsigned __int8		u8;
	typedef unsigned __int16	u16;
	typedef unsigned __int32	u32;
	typedef unsigned __int64	u64;
	typedef unsigned __int64	usize;
	typedef __int8				i8;
	typedef __int16				i16;
	typedef __int32				i32;
	typedef __int64				i64;
	typedef __int64				isize;
	typedef float				f32;
	typedef double				f64;
	typedef char				utf8;
	typedef	u32					utf32;
	typedef unsigned long		NRESULT;
}
