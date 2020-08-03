/**
 * Matrix.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Matrix.hpp"

namespace Re 
{
	namespace Math
	{
		NMatrix::NMatrix()
		{
			Memory::NMemSet(Elements, 0, 16 * sizeof(f32));
		}

		NMatrix::NMatrix(f32 InDiagonal)
			: NMatrix()
		{
			// Set Main Diagonal Using [Row + Column * 4]
			Elements[0 + 0 * 4] = InDiagonal;
			Elements[1 + 1 * 4] = InDiagonal;
			Elements[2 + 2 * 4] = InDiagonal;
			Elements[3 + 3 * 4] = InDiagonal;
		}

		NMatrix NMatrix::Multiply(const NMatrix& InOther) {
			NMatrix Result;

			for (u32 y = 0; y < 4; y++) {
				for (u32 x = 0; x < 4; x++) {
					f32 Sum = 0.0f;

					for (u32 e = 0; e < 4; e++) {
						Sum += Elements[x + e * 4] * InOther.Elements[e + y * 4];
					}

					Result.Elements[x + y * 4] = Sum;
				}
			}

			return Result;
		}

		NMatrix NMatrix::Identity(void) {
			return NMatrix(1.0f);
		}

		NMatrix NMatrix::Orthographic(f32 InLeft, f32 InRight, f32 InBottom, f32 InTop, f32 InNear, f32 InFar) {
			NMatrix Result = NMatrix::Identity();

			// Main Diagonal
			Result.Elements[0 + 0 * 4] = 2.0f / (InRight - InLeft);
			Result.Elements[1 + 1 * 4] = 2.0f / (InTop - InBottom);
			Result.Elements[2 + 2 * 4] = 2.0f / (InNear - InFar);

			// Last Column
			Result.Elements[0 + 3 * 4] = (InLeft + InRight) / (InLeft - InRight);
			Result.Elements[1 + 3 * 4] = (InBottom + InTop) / (InBottom - InTop);
			Result.Elements[2 + 3 * 4] = (InFar + InNear) / (InFar - InNear);

			return Result;
		}

		NMatrix NMatrix::Perspective(f32 AspectRatio, f32 FoV, f32 Near, f32 Far)
		{
			NMatrix Result;
			f32 Tangent = tanf(FoV / 2.0f);

			// Main Diagonal
			Result.Elements[0 + 0 * 4] = 1.0f / (AspectRatio * Tangent);
			Result.Elements[1 + 1 * 4] = 1.0f / Tangent;
			Result.Elements[2 + 2 * 4] = (Far + Near) / (Near - Far);

			// Off Diagonal
			Result.Elements[3 + 2 * 4] = -1.0f;
			Result.Elements[2 + 3 * 4] = (2.0f * Far * Near) / (Near - Far);

			return Result;
		}

		NMatrix NMatrix::LookAt(NVector3 Eye, NVector3 Center, NVector3 Up) {
			NMatrix Result = NMatrix::Identity();

			// Basic Vectors
			NVector3 A = Eye - Center;
			NVector3 B = Up;

			// Obtain Matrix Vectors
			NVector3 W = A.Normalize();
			NVector3 U = W.Cross(B).Normalize();
			NVector3 V = U.Cross(W);

			// First Column
			Result.Elements[0 + 0 * 4] = U.X;
			Result.Elements[1 + 0 * 4] = V.X;
			Result.Elements[2 + 0 * 4] = -W.X;

			// Second Column
			Result.Elements[0 + 1 * 4] = U.Y;
			Result.Elements[1 + 1 * 4] = V.Y;
			Result.Elements[2 + 1 * 4] = -W.Y;

			// Third Column
			Result.Elements[0 + 2 * 4] = U.Z;
			Result.Elements[1 + 2 * 4] = V.Z;
			Result.Elements[2 + 2 * 4] = -W.Z;

			// Fourth Column
			Result.Elements[0 + 3 * 4] = -U.Dot(Eye);
			Result.Elements[1 + 3 * 4] = -V.Dot(Eye);
			Result.Elements[2 + 3 * 4] = W.Dot(Eye);

			return Result;
		}

		NMatrix NMatrix::Rotation(f32 InAngle, const NVector3& InAxis) {
			NMatrix Result = NMatrix::Identity();
			f32 Radian = ToRadians(InAngle);
			f32 Cosine = cosf(Radian);
			f32 Sine = sinf(Radian);
			f32 OMC = 1.0f - Cosine;

			// First Column
			Result.Elements[0 + 0 * 4] = InAxis.X * InAxis.X * OMC + Cosine;
			Result.Elements[1 + 0 * 4] = InAxis.Y * InAxis.X * OMC + InAxis.Z * Sine;
			Result.Elements[2 + 0 * 4] = InAxis.X * InAxis.Z * OMC - InAxis.Y * Sine;

			// Second Column
			Result.Elements[0 + 1 * 4] = InAxis.X * InAxis.Y * OMC - InAxis.Z * Sine;
			Result.Elements[1 + 1 * 4] = InAxis.Y * InAxis.Y * OMC + Cosine;
			Result.Elements[2 + 1 * 4] = InAxis.Y * InAxis.Z * OMC + InAxis.X * Sine;

			// Third Column
			Result.Elements[0 + 2 * 4] = InAxis.X * InAxis.Z * OMC + InAxis.Y * Sine;
			Result.Elements[1 + 2 * 4] = InAxis.Y * InAxis.Z * OMC - InAxis.X * Sine;
			Result.Elements[2 + 2 * 4] = InAxis.Z * InAxis.Z * OMC + Cosine;

			return Result;
		}

		NMatrix NMatrix::Scale(const NVector3& InScale) {
			NMatrix Result = NMatrix::Identity();

			// Main Diagonal
			Result.Elements[0 + 0 * 4] = InScale.X;
			Result.Elements[1 + 1 * 4] = InScale.Y;
			Result.Elements[2 + 2 * 4] = InScale.Z;

			return Result;
		}

		NMatrix NMatrix::Translation(const NVector3& InTranslation) {
			NMatrix Result = NMatrix::Identity();

			// Last Column
			Result.Elements[0 + 3 * 4] = InTranslation.X;
			Result.Elements[1 + 3 * 4] = InTranslation.Y;
			Result.Elements[2 + 3 * 4] = InTranslation.Z;

			return Result;
		}

		NMatrix& NMatrix::operator*=(const NMatrix& InOther) { return Multiply(InOther); }
		NMatrix operator*(NMatrix InLeft, const NMatrix& InRight) { return InLeft.Multiply(InRight); }
	}
}
