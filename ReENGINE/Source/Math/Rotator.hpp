/**
 * Rotator.h
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

namespace Re
{
	namespace Math
	{
		/**
		 * @brief
		 *
		 */
		struct Rotator
		{
			f32 _pitch;
			f32 _roll;
			f32 _yaw;

			/**
			 * @brief
			 *
			 */
			inline Rotator()
				: _pitch(0.0f), _roll(0.0f), _yaw(0.0f) {}

			/**
			 * @brief
			 *
			 * @param f32 InF:
			 *
			 */
			explicit inline Rotator(f32 f)
				: _pitch(f), _roll(f), _yaw(f) {}

			/**
			 * @brief
			 *
			 * @param f32 InX:
			 * @param f32 InY:
			 * @param f32 InZ:
			 *
			 */
			explicit inline Rotator(f32 pitch, f32 roll, f32 yaw)
				: _pitch(pitch), _roll(roll), _yaw(yaw) {}
		};
	}
}
