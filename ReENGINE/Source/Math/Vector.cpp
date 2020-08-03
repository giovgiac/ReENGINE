/**
 * Vector.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Vector.hpp"
#include "Vector3.hpp"

#include <math.h>

namespace Re 
{
	namespace Math
	{
		NVector::NVector()
			: X(0.0f), Y(0.0f) {}

		NVector::NVector(const f32 InF)
			: X(InF), Y(InF) {}

		NVector::NVector(const f32 InX, const f32 InY)
			: X(InX), Y(InY) {}

		NVector NVector::Add(const NVector& InOther) {
			NVector Result;

			Result.X = X + InOther.X;
			Result.Y = Y + InOther.Y;
			return Result;
		}

		NVector NVector::Divide(const f32 InScalar) {
			ASSERT(InScalar != 0);

			NVector Result;

			Result.X = X / InScalar;
			Result.Y = Y / InScalar;
			return Result;
		}

		NVector NVector::Multiply(const f32 InScalar) {
			NVector Result;

			Result.X = X * InScalar;
			Result.Y = Y * InScalar;
			return Result;
		}

		bool NVector::operator==(const NVector& InOther) { return (X == InOther.X) && (Y == InOther.Y); }

		NVector& NVector::operator+=(const NVector& InOther) { 
			Add(InOther);
			return *this;
		}

		NVector& NVector::operator*=(const f32 InScalar) { 
			Multiply(InScalar);
			return *this;
		}

		NVector& NVector::operator/=(const f32 InScalar) { 
			Divide(InScalar);
			return *this;
		}

		NVector operator+(NVector InLeft, const NVector& InRight) { return InLeft.Add(InRight); }
		NVector operator*(NVector InVector, const f32 InScalar) { return InVector.Multiply(InScalar); }
		NVector operator/(NVector InVector, const f32 InScalar) { return InVector.Divide(InScalar); }
	}
}
