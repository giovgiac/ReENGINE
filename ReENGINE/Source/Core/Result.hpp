/*
 * Result.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

namespace Re
{
	#define CHECK_RESULT(value, expected, ret)							\
		if (value != expected)											\
		{																\
			return ret;													\
		}

	#define CHECK_RESULT_WITH_WARNING(value, expected, msg, ret)		\
		if (value != expected)											\
		{																\
			Core::Debug::Warning(msg);									\
			return ret;													\
		}

	#define CHECK_RESULT_WITH_ERROR(value, expected, msg, ret)			\
		if (value != expected)											\
		{																\
			Core::Debug::Error(msg);									\
			return ret;													\
		}
}
