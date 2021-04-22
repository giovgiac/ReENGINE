/**
 * Vector3.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Vector3.hpp"

#include <math.h>

namespace Re
{
	namespace Math
	{
		Vector3& Vector3::Add(const Vector3& InOther)
		{
			// Perform element-wise vector addition.
			X += InOther.X;
			Y += InOther.Y;
			Z += InOther.Z;

			return *this;
		}

		Vector3& Vector3::Cross(const Vector3& InOther)
		{
			Vector3 Result;

			// Perform three-dimensional vector cross product.
			Result.X = Y * InOther.Z - Z * InOther.Y;
			Result.Y = Z * InOther.X - X * InOther.Z;
			Result.Z = X * InOther.Y - Y * InOther.X;
			*this = Result;

			return *this;
		}

		f32 Vector3::Dot(const Vector3& InOther) const 
		{
			return (X * InOther.X + Y * InOther.Y + Z * InOther.Z);
		}

		Vector3& Vector3::Divide(const f32 InScalar) 
		{
			ASSERT(InScalar != 0);

			// Perform element-wise vector division.
			X /= InScalar;
			Y /= InScalar;
			Z /= InScalar;

			return *this;
		}

		f32 Vector3::Length() const 
		{
			return sqrtf(powf(X, 2) + powf(Y, 2) + powf(Z, 2));
		}

		Vector3& Vector3::Normalize() 
		{
			f32 Magnitude = Length();

			X /= Magnitude;
			Y /= Magnitude;
			Z /= Magnitude;
			return *this;
		}

		Vector3& Vector3::Multiply(const f32 InScalar) 
		{
			// Perform element-wise vector multiplication.
			X *= InScalar;
			Y *= InScalar;
			Z *= InScalar;

			return *this;
		}

		Vector3& Vector3::Subtract(const Vector3& InOther) 
		{
			// Perform element-wise vector subtraction.
			X -= InOther.X;
			Y -= InOther.Y;
			Z -= InOther.Z;

			return *this;
		}

		Vector3 Vector3::Cross(const Vector3& InLeft, const Vector3& InRight) 
		{
			return Vector3(InLeft.Y * InRight.Z - InLeft.Z * InRight.Y, InLeft.Z * InRight.X - InLeft.X * InRight.Z, InLeft.X * InRight.Y - InLeft.Y * InRight.X);
		}

		Vector3& Vector3::operator+=(const Vector3& InOther) { return Add(InOther); }
		Vector3& Vector3::operator-=(const Vector3& InOther) { return Subtract(InOther); }
		Vector3& Vector3::operator*=(const f32 InScalar) { return Multiply(InScalar); }
		Vector3& Vector3::operator/=(const f32 InScalar) { return Divide(InScalar); }

		bool Vector3::operator==(const Vector3& InOther)
		{
			return	X == InOther.X && Y == InOther.Y && Z == InOther.Z;
		}

		bool Vector3::operator!=(const Vector3& InOther)
		{
			return !this->operator==(InOther);
		}

		Vector3 Vector3::operator-() const
		{
			Vector3 Result;

			// Invert individual components.
			Result.X = -X;
			Result.Y = -Y;
			Result.Z = -Z;

			return Result;
		}

		Vector3 operator+(Vector3 InLeft, const Vector3& InRight) { return InLeft.Add(InRight); }
		Vector3 operator-(Vector3 InLeft, const Vector3& InRight) { return InLeft.Subtract(InRight); }
		Vector3 operator*(Vector3 InVector, const f32 InScalar) { return InVector.Multiply(InScalar); }
		Vector3 operator/(Vector3 InVector, const f32 InScalar) { return InVector.Divide(InScalar); }
	}
}
