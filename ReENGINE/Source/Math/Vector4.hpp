/**
 * Vector4.h
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
		/*
		 * @brief This data type holds a vector in 4-D space. It uses floating-point numbers for storing 
		 * the coordinates.
		 *
		 */
		struct Vector4 
		{
			f32 X;
			f32 Y;
			f32 Z;
			f32 W;

			/**
			 * @brief This default constructor initializes the vector to the zero vector.
			 *
			 */
			inline Vector4(void)
				: X(0.0f), Y(0.0f), Z(0.0f), W(0.0f) {}

			/**
			 * @brief This constructor initializes all coordinates in the vector with the same given floating-point value.
			 *
			 * @param f32 InF: The value to fill the coordinates with.
			 *
			 */
			explicit inline Vector4(f32 InF)
				: X(InF), Y(InF), Z(InF), W(InF) {}

			/*
			 * @brief This constructor initializes a vector with the given floating-point coordinates.
			 *
			 * @param InX: the value of the X coordinate.
			 * @param InY: the value of the Y coordinate.
			 * @param InZ: the value of the Z coordinate.
			 * @param InW: the value of the W coordinate.
			 *
			 */
			explicit inline Vector4(f32 InX, f32 InY, f32 InZ, f32 InW)
				: X(InX), Y(InY), Z(InZ), W(InW) {}
		};
	}
}
