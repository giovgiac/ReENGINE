/**
 * Math.h
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"
#include <math.h>

namespace Re 
{
	namespace Math
	{
		const f32 PI = 3.141592654f;

		/*
		 * @brief This function converts an angle value from degrees to radians.
		 *
		 * @param InAngle: the value of the angle in degrees.
		 *
		 * @return the value of the angle in radians.
		 *
		 */
		extern f32 ToRadians(f32 InAngle);

		/*
		 * @brief This function converts an angle value from radians to degrees.
		 *
		 * @param InAngle: the value of the angle in radians.
		 *
		 * @return the value of the angle in degrees.
		 *
		 */
		extern f32 ToDegrees(f32 InAngle);
	}
}
