/*
 * Matrix.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Memory/Memory.hpp"
#include "Math.hpp"
#include "Vector3.hpp"
#include "Vector4.hpp"

namespace Re
{
	namespace Math
	{
		/*
		 * @brief This data type holds a 4x4 Matrix in column-major ordering and using as values floating-point numbers.
		 *
		 */
		struct NMatrix
		{
			union 
			{
				NVector4 Columns[4];
				f32 Elements[16];
				f32 Transpose[4][4];
			};

			/*
			 * @brief This default constructor initializes a matrix with all entries holding zero.
			 *
			 */
			NMatrix();

			/*
			 * @brief This constructor initializes a matrix with the main diagonal holding the given value and everywhere else
			 * holding zero.
			 *
			 * @param InDiagonal: the value of the main diagonal entries.
			 *
			 */
			explicit NMatrix(f32 InDiagonal);

			/*
			 * @brief This method multiplies the current matrix by another given matrix.
			 *
			 * @param InOther: the matrix to multiply by.
			 *
			 * @return a reference to the current matrix.
			 *
			 */
			NMatrix Multiply(const NMatrix& InOther);

			/*
			 * @brief This static method initializes a matrix with the main diagonal holding one and everywhere else
			 * holding zero.
			 *
			 * @return a initialized identity matrix.
			 *
			 */
			static NMatrix Identity(void);

			/*
			 * @brief This static method initializes a projection matrix of type Orthographic given the clip values.
			 *
			 * @param InLeft: the value to clip at left.
			 * @param InRight: the value to clip at right.
			 * @param InBottom: the value to clip at bottom.
			 * @param InTop: the value to clip at top.
			 * @param InNear: the value to clip at near.
			 * @param InFar: the value to clip at far.
			 *
			 * @return a initialized orthographic projection matrix.
			 *
			 */
			static NMatrix Orthographic(f32 InLeft, f32 InRight, f32 InBottom, f32 InTop, f32 InNear, f32 InFar);

			static NMatrix Perspective(f32 AspectRatio, f32 FoV, f32 Near, f32 Far);

			/*
			 *
			 */
			static NMatrix LookAt(NVector3 Eye, NVector3 Center, NVector3 Up);

			/*
			 * @brief This static method initializes a rotation matrix given the angle and axis of rotation.
			 *
			 * @param InAngle: the value of the angle to rotate by.
			 * @param InAxis: the axis to rotate around.
			 *
			 * @return a initialized rotation matrix.
			 *
			 */
			static NMatrix Rotation(f32 InAngle, const NVector3& InAxis);

			/*
			 * @brief This static method initializes a scale matrix given the values to scale each component by.
			 *
			 * @param InScale: the values to scale each component by.
			 *
			 * @return a initialized scaling matrix.
			 *
			 */
			static NMatrix Scale(const NVector3& InScale);

			/*
			 * @brief This static method initializes a translation matrix given the values to move each component by.
			 *
			 * @param InTranslation: the values to move each component by.
			 *
			 * @return a initialized translation matrix.
			 *
			 */
			static NMatrix Translation(const NVector3& InTranslation);

			INLINE NMatrix& operator*=(const NMatrix& InOther);
			friend NMatrix operator*(NMatrix InLeft, const NMatrix& InRight);
		};
	}
}
