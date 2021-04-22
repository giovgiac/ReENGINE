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
		Matrix::Matrix()
		{
			Memory::NMemSet(Elements, 0, 16 * sizeof(f32));
		}

		Matrix::Matrix(f32 InDiagonal)
			: Matrix()
		{
			// Set Main Diagonal Using [Column * 4 + Row]
			Elements[0 * 4 + 0] = InDiagonal;
			Elements[1 * 4 + 1] = InDiagonal;
			Elements[2 * 4 + 2] = InDiagonal;
			Elements[3 * 4 + 3] = InDiagonal;
		}

		Matrix Matrix::Multiply(const Matrix& InOther) {
			Matrix Result;

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

		Matrix Matrix::Identity(void) {
			return Matrix(1.0f);
		}

		Matrix Matrix::Orthographic(f32 InLeft, f32 InRight, f32 InBottom, f32 InTop, f32 InNear, f32 InFar) {
			Matrix Result = Matrix::Identity();

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

		Matrix Matrix::Perspective(f32 AspectRatio, f32 FoV, f32 Near, f32 Far)
		{
			Matrix Result;

			// Calculate auxiliary values.
			FoV = ToRadians(FoV);
			f32 Tangent = tanf(FoV / 2.0f);

			// Main Diagonal
			Result.Elements[0 * 4 + 0] = 1.0f / (AspectRatio * Tangent);
			Result.Elements[1 * 4 + 1] = -1.0f / Tangent;
			Result.Elements[2 * 4 + 2] = -(Far + Near) / (Far - Near);

			// Off Diagonal
			Result.Elements[2 * 4 + 3] = -1.0f;
			Result.Elements[3 * 4 + 2] = -(2.0f * Far * Near) / (Far - Near);

			return Result;
		}

		Matrix Matrix::LookAt(const Vector3& Eye, const Vector3& Center, const Vector3& Up) {
			Matrix Result = Matrix::Identity();

			// Basic Vectors
			Vector3 F = (Center - Eye).Normalize();
			Vector3 S = Vector3::Cross(F, Up).Normalize();
			Vector3 U = Vector3::Cross(S, F);

			// First Row
			Result.Elements[0 * 4 + 0] = +S.X;
			Result.Elements[1 * 4 + 0] = +S.Y;
			Result.Elements[2 * 4 + 0] = +S.Z;

			// Second Row
			Result.Elements[0 * 4 + 1] = +U.X;
			Result.Elements[1 * 4 + 1] = +U.Y;
			Result.Elements[2 * 4 + 1] = +U.Z;

			// Third Row
			Result.Elements[0 * 4 + 2] = -F.X;
			Result.Elements[1 * 4 + 2] = -F.Y;
			Result.Elements[2 * 4 + 2] = -F.Z;

			// Fourth Column
			Result.Elements[3 * 4 + 0] = -S.Dot(Eye);
			Result.Elements[3 * 4 + 1] = -U.Dot(Eye);
			Result.Elements[3 * 4 + 2] = +F.Dot(Eye);

			return Result;
		}

		Matrix Matrix::Rotation(f32 InAngle, const Vector3& InAxis) {
			Matrix Result = Matrix::Identity();
			f32 Radian = ToRadians(InAngle);
			f32 Cosine = cosf(Radian);
			f32 Sine = sinf(Radian);
			f32 OMC = 1.0f - Cosine;

			// First Column
			Result.Elements[0 * 4 + 0] = InAxis.X * InAxis.X * OMC + Cosine;
			Result.Elements[0 * 4 + 1] = InAxis.Y * InAxis.X * OMC + InAxis.Z * Sine;
			Result.Elements[0 * 4 + 2] = InAxis.X * InAxis.Z * OMC - InAxis.Y * Sine;

			// Second Column
			Result.Elements[1 * 4 + 0] = InAxis.X * InAxis.Y * OMC - InAxis.Z * Sine;
			Result.Elements[1 * 4 + 1] = InAxis.Y * InAxis.Y * OMC + Cosine;
			Result.Elements[1 * 4 + 2] = InAxis.Y * InAxis.Z * OMC + InAxis.X * Sine;

			// Third Column
			Result.Elements[2 * 4 + 0] = InAxis.X * InAxis.Z * OMC + InAxis.Y * Sine;
			Result.Elements[2 * 4 + 1] = InAxis.Y * InAxis.Z * OMC - InAxis.X * Sine;
			Result.Elements[2 * 4 + 2] = InAxis.Z * InAxis.Z * OMC + Cosine;

			return Result;
		}

		Matrix Matrix::Scale(const Vector3& InScale) {
			Matrix Result = Matrix::Identity();

			// Main Diagonal
			Result.Elements[0 * 4 + 0] = InScale.X;
			Result.Elements[1 * 4 + 1] = InScale.Y;
			Result.Elements[2 * 4 + 2] = InScale.Z;

			return Result;
		}

		Matrix Matrix::Translation(const Vector3& InTranslation) {
			Matrix Result = Matrix::Identity();

			// Last Column
			Result.Elements[3 * 4 + 0] = InTranslation.X;
			Result.Elements[3 * 4 + 1] = InTranslation.Y;
			Result.Elements[3 * 4 + 2] = InTranslation.Z;

			return Result;
		}

		Matrix& Matrix::operator*=(const Matrix& InOther) { return Multiply(InOther); }
		Matrix operator*(Matrix InLeft, const Matrix& InRight) { return InLeft.Multiply(InRight); }
	}
}
