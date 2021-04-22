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
		/**
		 * @brief This data type is for holding a vector in 3-D space. It uses floating-point numbers for 
		 * the coordinates.
		 *
		 */
		struct Vector3 
		{
			f32 X;
			f32 Y;
			f32 Z;

			/**
			 * @brief This default constructor initializes the vector to the zero vector.
			 *
			 */
			inline Vector3()
				: X(0.0f), Y(0.0f), Z(0.0f) {}

			/**
			 * @brief This constructor initializes the coordinates of the vector to the same given floating-point number.
			 *
			 * @param f32 InF: The value to initialize the coordinates with.
			 *
			 */
			explicit inline Vector3(f32 InF)
				: X(InF), Y(InF), Z(InF) {}

			/**
			 * @brief This constructor initializes the coordinates of the vector to the given floating-point numbers.
			 *
			 * @param f32 InX: The value of the X coordinate.
			 * @param f32 InY: The value of the Y coordinate.
			 * @param f32 InZ: The value of the Z coordinate.
			 *
			 */
			explicit inline Vector3(f32 InX, f32 InY, f32 InZ)
				: X(InX), Y(InY), Z(InZ) {}

			/**
			 * @brief This method adds two vectors together.
			 *
			 * @param const Vector3& InOther: The vector to add.
			 *
			 * @return Vector3&: A reference to self.
			 *
			 */
			Vector3& Add(const Vector3& InOther);

			/**
			 * @brief This method does a vector-to-vector product between two vectors.
			 *
			 * @param const Vector3& InOther: The vector to cross-product with.
			 *
			 * @return Vector3&: A reference to self.
			 *
			 */
			Vector3& Cross(const Vector3& InOther);

			/**
			 * @brief This method divides a vector by a scalar.
			 *
			 * @param const f32 InScalar: The scalar to divide the vector by.
			 *
			 * @return Vector3&: A reference to self.
			 *
			 */
			Vector3& Divide(const f32 InScalar);

			/**
			 * @brief This method does a scalar product between two vectors.
			 *
			 * @param const Vector3& InOther: The vector to dot-product with.
			 *
			 * @return f32: The result of the dot product.
			 *
			 */
			f32 Dot(const Vector3& InOther) const;

			/**
			 * @brief This method calculates the length of the vector.
			 *
			 * @return f32: The magnitude of the vector.
			 *
			 */
			f32 Length() const;

			/**
			 * @brief This method multiplies a vector by a given scalar.
			 *
			 * @param const f32 InScalar: The scalar to multiply the vector by.
			 *
			 * @return Vector3&: A reference to self.
			 *
			 */
			Vector3& Multiply(const f32 InScalar);

			/**
			 * @brief This method normalizes the vector by dividing through by it's length.
			 *
			 * @return Vector3&: A reference to self.
			 *
			 */
			Vector3& Normalize();

			/**
			 * @brief This method subtracts a vector by another.
			 *
			 * @param const Vector3& InOther: The vector to subtract by.
			 *
			 * @return Vector3&: A reference to self.
			 *
			 */
			Vector3& Subtract(const Vector3& InOther);

			/**
			 * @brief This method statically does the cross-product between two given vectors.
			 *
			 * @param const Vector3& InLeft: The left vector of the operation.
			 * @param const Vector3& InRight: The right vector of the operation.
			 *
			 * @return Vector3: A copy of the resulting vector.
			 *
			 */
			static Vector3 Cross(const Vector3& InLeft, const Vector3& InRight);

			Vector3& operator+=(const Vector3& InOther);
			Vector3& operator-=(const Vector3& InOther);
			Vector3& operator*=(const f32 InScalar);
			Vector3& operator/=(const f32 InScalar);

			bool operator==(const Vector3& InOther);
			bool operator!=(const Vector3& InOther);

			Vector3 operator-() const;

			friend Vector3 operator+(Vector3 InLeft, const Vector3& InRight);
			friend Vector3 operator-(Vector3 InLeft, const Vector3& InRight);
			friend Vector3 operator*(Vector3 InVector, const f32 InScalar);
			friend Vector3 operator/(Vector3 InVector, const f32 InScalar);
		};

		const Vector3 WorldUp(0.0f, 1.0f, 0.0f);
	}
}
