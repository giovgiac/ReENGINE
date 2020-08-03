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
		NVector3 NVector3::Add(const NVector3& InOther)
		{
			NVector3 Result;

			Result.X = X + InOther.X;
			Result.Y = Y + InOther.Y;
			Result.Z = Z + InOther.Z;
			return Result;
		}

		NVector3 NVector3::Cross(const NVector3& InOther)
		{
			NVector3 Result;

			Result.X = Y * InOther.Z - Z * InOther.Y;
			Result.Y = Z * InOther.X - X * InOther.Z;
			Result.Z = X * InOther.Y - Y * InOther.X;
			return Result;
		}

		f32 NVector3::Dot(const NVector3& InOther) const {
			return (X * InOther.X + Y * InOther.Y + Z * InOther.Z);
		}

		NVector3 NVector3::Divide(const f32 InScalar) {
			ASSERT(InScalar != 0);

			NVector3 Result;

			Result.X = X / InScalar;
			Result.Y = Y / InScalar;
			Result.Z = Z / InScalar;
			return Result;
		}

		f32 NVector3::Length() const {
			return sqrtf(powf(X, 2) + powf(Y, 2) + powf(Z, 2));
		}

		NVector3 NVector3::Normalize() {
			f32 Magnitude = Length();
			NVector3 Result;

			Result.X = X / Magnitude;
			Result.Y = Y / Magnitude;
			Result.Z = Z / Magnitude;
			return Result;
		}

		NVector3 NVector3::Multiply(const f32 InScalar) {
			NVector3 Result;

			Result.X = X * InScalar;
			Result.Y = Y * InScalar;
			Result.Z = Z * InScalar;
			return Result;
		}

		NVector3 NVector3::Subtract(const NVector3& InOther) {
			NVector3 Result;

			Result.X = X - InOther.X;
			Result.Y = Y - InOther.Y;
			Result.Z = Z - InOther.Z;
			return Result;
		}

		NVector3 NVector3::Cross(const NVector3& InLeft, const NVector3& InRight) {
			return NVector3(InLeft.Y * InRight.Z - InLeft.Z * InRight.Y, InLeft.Z * InRight.X - InLeft.X * InRight.Z, InLeft.X * InRight.Y - InLeft.Y * InRight.X);
		}

		NVector3& NVector3::operator+=(const NVector3& InOther) { return Add(InOther); }
		NVector3& NVector3::operator-=(const NVector3& InOther) { return Subtract(InOther); }
		NVector3& NVector3::operator*=(const f32 InScalar) { return Multiply(InScalar); }
		NVector3& NVector3::operator/=(const f32 InScalar) { return Divide(InScalar); }

		NVector3 operator+(NVector3 InLeft, const NVector3& InRight) { return InLeft.Add(InRight); }
		NVector3 operator-(NVector3 InLeft, const NVector3& InRight) { return InLeft.Subtract(InRight); }
		NVector3 operator*(NVector3 InVector, const f32 InScalar) { return InVector.Multiply(InScalar); }
		NVector3 operator/(NVector3 InVector, const f32 InScalar) { return InVector.Divide(InScalar); }
	}
}
