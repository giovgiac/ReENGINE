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
		Vector::Vector()
			: X(0.0f), Y(0.0f) {}

		Vector::Vector(const f32 InF)
			: X(InF), Y(InF) {}

		Vector::Vector(const f32 InX, const f32 InY)
			: X(InX), Y(InY) {}

		Vector Vector::Add(const Vector& InOther) {
			Vector Result;

			Result.X = X + InOther.X;
			Result.Y = Y + InOther.Y;
			return Result;
		}

		Vector Vector::Divide(const f32 InScalar) {
			ASSERT(InScalar != 0);

			Vector Result;

			Result.X = X / InScalar;
			Result.Y = Y / InScalar;
			return Result;
		}

		Vector Vector::Multiply(const f32 InScalar) {
			Vector Result;

			Result.X = X * InScalar;
			Result.Y = Y * InScalar;
			return Result;
		}

		bool Vector::operator==(const Vector& InOther) { return (X == InOther.X) && (Y == InOther.Y); }

		bool Vector::operator!=(const Vector& InOther)
		{
			return !this->operator==(InOther);
		}

		Vector& Vector::operator+=(const Vector& InOther) { 
			Add(InOther);
			return *this;
		}

		Vector& Vector::operator*=(const f32 InScalar) { 
			Multiply(InScalar);
			return *this;
		}

		Vector& Vector::operator/=(const f32 InScalar) { 
			Divide(InScalar);
			return *this;
		}

		Vector operator+(Vector InLeft, const Vector& InRight) { return InLeft.Add(InRight); }
		Vector operator*(Vector InVector, const f32 InScalar) { return InVector.Multiply(InScalar); }
		Vector operator/(Vector InVector, const f32 InScalar) { return InVector.Divide(InScalar); }
	}
}
