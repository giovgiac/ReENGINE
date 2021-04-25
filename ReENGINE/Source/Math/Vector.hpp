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
		struct Vector 
		{
			f32 X;
			f32 Y;

			/**
			 * Vector Constructor
			 *
			 * The default constructor which initializes the vector to the zero vector.
			 *
			 */
			Vector();

			/**
			 * Vector Constructor
			 *
			 * This constructor initializes the vector with components equal to the same floating-point number.
			 *
			 * @param f32 InF: The value to initialize the components to.
			 *
			 */
			Vector(const f32 InF);

			/**
			 * Vector Constructor
			 *
			 * This constructor initializes the vector to given floating-point values.
			 *
			 * @param f32 InX: The value of the X coordinate.
			 * @param f32 InY: The value of the Y coordinate.
			 *
			 */
			Vector(const f32 InX, const f32 InY);

			/**
			 * Vector Add
			 *
			 * This method adds two vectors together, the current one with the given one.
			 *
			 * @param const Vector& InOther: The vector add with the current one.
			 *
			 * @return Vector&: A reference to self.
			 *
			 */
			Vector Add(const Vector& InOther);

			/**
			 * Vector Divide
			 *
			 * This method divides a vector by a scalar.
			 *
			 * @param const f32 InScalar: The scalar to divide the vector by.
			 *
			 * @return Vector&: A reference to self.
			 *
			 */
			Vector Divide(const f32 InScalar);

			/**
			 * Vector Multiply
			 *
			 * This method multiplies a vector and a scalar.
			 *
			 * @param const f32 InScalar: The scalar to multiply the vector by.
			 *
			 * @return Vector&: A reference to self.
			 *
			 */
			Vector Multiply(const f32 InScalar);

			/**
			 * Vector Zero
			 *
			 * This method returns the zero vector.
			 *
			 * @return Vector: A copy of the zero vector.
			 *
			 */
			inline static Vector Zero() { return Vector(0.0f); }

			bool operator==(const Vector& InOther);
			bool operator!=(const Vector& InOther);

			Vector& operator+=(const Vector& InOther);
			Vector& operator*=(const f32 InScalar);
			Vector& operator/=(const f32 InScalar);

			friend Vector operator+(Vector InLeft, const Vector& InRight);
			friend Vector operator*(Vector InVector, const f32 InScalar);
			friend Vector operator/(Vector InVector, const f32 InScalar);
		};
	}
}
