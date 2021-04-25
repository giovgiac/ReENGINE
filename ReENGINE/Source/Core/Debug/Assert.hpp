/*
 * Assert.hpp
 *
 * This header file defines an ASSERT macro which is used
 * throughout the ReENGINE's code to verify the programmer's assumption
 * didn't fail, especially when altering code.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Debug.hpp"

#ifdef ASSERTIONS

#define AssertBreak() // _asm { int 3 }

/*
 * @brief This function uses a default or ReENGINE's logging system to
 * print debugging and error information just before the ASSERT macro
 * launches the VS debugger.
 *
 * @param pExpression: pointer to the string that represents the expression.
 * @param pFile: pointer to the string that contains the filename.
 * @param line: the unsigned integer that holds the line.
 *
 */
void ReportAssertFailure(const utf8* pExpression, const utf8* pFile, u32 line);

#define ASSERT(expr) \
		if (expr) {} \
		else \
		{ \
			ReportAssertFailure(#expr, __FILE__, __LINE__); \
			AssertBreak(); \
		}

#else

#define ASSERT(expr) // Do Nothing...

#endif
