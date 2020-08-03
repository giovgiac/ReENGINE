/**
 * Vector3.h
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
		 * @brief This data type is for holding a vector in 3-D space. It uses floating-point numbers for 
		 * the coordinates.
		 *
		 */
		struct NVector3 
		{
			f32 X;
			f32 Y;
			f32 Z;

			/**
			 * NVector3 Constructor
			 *
			 * This default constructor initializes the vector to the zero vector.
			 *
			 */
			inline NVector3()
				: X(0.0f), Y(0.0f), Z(0.0f) {}

			/**
			 * NVector3 Constructor
			 *
			 * This constructor initializes the coordinates of the vector to the same given floating-point number.
			 *
			 * @param f32 InF: The value to initialize the coordinates with.
			 *
			 */
			explicit inline NVector3(f32 InF)
				: X(InF), Y(InF), Z(InF) {}

			/**
			 * NVector3 Constructor
			 *
			 * This constructor initializes the coordinates of the vector to the given floating-point numbers.
			 *
			 * @param f32 InX: The value of the X coordinate.
			 * @param f32 InY: The value of the Y coordinate.
			 * @param f32 InZ: The value of the Z coordinate.
			 *
			 */
			explicit inline NVector3(f32 InX, f32 InY, f32 InZ)
				: X(InX), Y(InY), Z(InZ) {}

			/**
			 * NVector3 Add
			 *
			 * This method adds two vectors together.
			 *
			 * @param const NVector3& InOther: The vector to add.
			 *
			 * @return NVector3&: A reference to self.
			 *
			 */
			NVector3 Add(const NVector3& InOther);

			/**
			 * NVector3 Cross
			 *
			 * This method does a vector-to-vector product between two vectors.
			 *
			 * @param const NVector3& InOther: The vector to cross-product with.
			 *
			 * @return NVector3&: A reference to self.
			 *
			 */
			NVector3 Cross(const NVector3& InOther);

			/**
			 * NVector3 Divide
			 *
			 * This method divides a vector by a scalar.
			 *
			 * @param const f32 InScalar: The scalar to divide the vector by.
			 *
			 * @return NVector3&: A reference to self.
			 *
			 */
			NVector3 Divide(const f32 InScalar);

			/**
			 * NVector3 Dot
			 *
			 * This method does a scalar product between two vectors.
			 *
			 * @param const NVector3& InOther: The vector to dot-product with.
			 *
			 * @return f32: The result of the dot product.
			 *
			 */
			f32 Dot(const NVector3& InOther) const;

			/**
			 * NVector3 Length
			 *
			 * This method calculates the length of the vector.
			 *
			 * @return f32: The magnitude of the vector.
			 *
			 */
			f32 Length() const;

			/**
			 * NVector3 Multiply
			 *
			 * This method multiplies a vector by a given scalar.
			 *
			 * @param const f32 InScalar: The scalar to multiply the vector by.
			 *
			 * @return NVector3&: A reference to self.
			 *
			 */
			NVector3 Multiply(const f32 InScalar);

			/**
			 * NVector3 Normalize
			 *
			 * This method normalizes the vector by dividing through by it's length.
			 *
			 * @return NVector3&: A reference to self.
			 *
			 */
			NVector3 Normalize();

			/**
			 * NVector3 Subtract
			 *
			 * This method subtracts a vector by another.
			 *
			 * @param const NVector3& InOther: The vector to subtract by.
			 *
			 * @return NVector3&: A reference to self.
			 *
			 */
			NVector3 Subtract(const NVector3& InOther);

			/**
			 * NVector3 Cross
			 *
			 * This method statically does the cross-product between two given vectors.
			 *
			 * @param const NVector3& InLeft: The left vector of the operation.
			 * @param const NVector3& InRight: The right vector of the operation.
			 *
			 * @return NVector3: A copy of the resulting vector.
			 *
			 */
			static NVector3 Cross(const NVector3& InLeft, const NVector3& InRight);

			NVector3& operator+=(const NVector3& InOther);
			NVector3& operator-=(const NVector3& InOther);
			NVector3& operator*=(const f32 InScalar);
			NVector3& operator/=(const f32 InScalar);

			friend NVector3 operator+(NVector3 InLeft, const NVector3& InRight);
			friend NVector3 operator-(NVector3 InLeft, const NVector3& InRight);
			friend NVector3 operator*(NVector3 InVector, const f32 InScalar);
			friend NVector3 operator/(NVector3 InVector, const f32 InScalar);
		};
	}
}
