/**
 * Math.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Math.hpp"

namespace Re 
{
	namespace Math
	{
		f32 ToRadians(f32 InAngle) {
			return (InAngle * PI / 180.0f);
		}

		f32 ToDegrees(f32 InAngle) {
			return (InAngle * 180.0f / PI);
		}
	}
}
