/*
 * Vector.h
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
		 * @brief This data type is for holding a vector in 2-D space, using floating-point coordinates.
		 *
		 */
		struct NVector 
		{
			f32 X;
			f32 Y;

			/**
			 * NVector Constructor
			 *
			 * The default constructor which initializes the vector to the zero vector.
			 *
			 */
			NVector();

			/**
			 * NVector Constructor
			 *
			 * This constructor initializes the vector with components equal to the same floating-point number.
			 *
			 * @param f32 InF: The value to initialize the components to.
			 *
			 */
			explicit NVector(const f32 InF);

			/**
			 * NVector Constructor
			 *
			 * This constructor initializes the vector to given floating-point values.
			 *
			 * @param f32 InX: The value of the X coordinate.
			 * @param f32 InY: The value of the Y coordinate.
			 *
			 */
			explicit NVector(const f32 InX, const f32 InY);

			/**
			 * NVector Add
			 *
			 * This method adds two vectors together, the current one with the given one.
			 *
			 * @param const NVector& InOther: The vector add with the current one.
			 *
			 * @return NVector&: A reference to self.
			 *
			 */
			NVector Add(const NVector& InOther);

			/**
			 * NVector Divide
			 *
			 * This method divides a vector by a scalar.
			 *
			 * @param const f32 InScalar: The scalar to divide the vector by.
			 *
			 * @return NVector&: A reference to self.
			 *
			 */
			NVector Divide(const f32 InScalar);

			/**
			 * NVector Multiply
			 *
			 * This method multiplies a vector and a scalar.
			 *
			 * @param const f32 InScalar: The scalar to multiply the vector by.
			 *
			 * @return NVector&: A reference to self.
			 *
			 */
			NVector Multiply(const f32 InScalar);

			/**
			 * NVector Zero
			 *
			 * This method returns the zero vector.
			 *
			 * @return NVector: A copy of the zero vector.
			 *
			 */
			inline static NVector Zero() { return NVector(0.0f); }

			bool operator==(const NVector& InOther);

			NVector& operator+=(const NVector& InOther);
			NVector& operator*=(const f32 InScalar);
			NVector& operator/=(const f32 InScalar);

			friend NVector operator+(NVector InLeft, const NVector& InRight);
			friend NVector operator*(NVector InVector, const f32 InScalar);
			friend NVector operator/(NVector InVector, const f32 InScalar);
		};
	}
}
