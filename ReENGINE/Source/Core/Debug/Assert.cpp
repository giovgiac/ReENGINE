/*
 * Assertion.cpp
 *
 * This source file implements functions defined in the
 * Assertion.hpp header file.
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Assert.hpp"

#ifdef ASSERTIONS

using namespace Re::Core;

void ReportAssertFailure(const utf8* pExpression, const utf8* pFile, u32 line) 
{
	Debug::Log(NTEXT("ASSERT FAILURE: (%s) at file %s in line %d\n"), pExpression, pFile, line);
}

#endif
