/**
 * Transform.h
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Vector.hpp"

namespace Re
{
	namespace Math
	{
		struct NTransform
		{
			NVector Position;
			float Rotation;
			NVector Scale;
		};
	}
}
