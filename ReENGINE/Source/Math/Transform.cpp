/**
 * Transform.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Transform.hpp"

namespace Re
{
	namespace Math
	{
		Vector3 Transform::Forward() const
		{
			return Vector3(
				  cosf(ToRadians(_rotation._pitch)) * sinf(ToRadians(_rotation._yaw)),
				  sinf(ToRadians(_rotation._pitch)),
				- cosf(ToRadians(_rotation._pitch)) * cosf(ToRadians(_rotation._yaw))
			).Normalize();
		}

		Vector3 Transform::Right() const
		{
			return Vector3::Cross(
				Forward(),
				Math::WorldUp
			).Normalize();
		}

		Vector3 Transform::Up() const
		{
			return Vector3::Cross(
				Right(), 
				Forward()
			).Normalize();
		}

		Matrix Transform::ToModel() const
		{
			// Query for individual rotations around each of the euclidean axes.
			Matrix rotationTransformX = Matrix::Rotation(_rotation._roll, Vector3(1.0f, 0.0f, 0.0f));
			Matrix rotationTransformY = Matrix::Rotation(_rotation._yaw, Vector3(0.0f, 1.0f, 0.0f));
			Matrix rotationTransformZ = Matrix::Rotation(_rotation._pitch, Vector3(0.0f, 0.0f, 1.0f));

			// Produce and query the transformation matrices.
			Matrix rotationTransform = rotationTransformX * rotationTransformY * rotationTransformZ;
			Matrix positionTransform = Matrix::Translation(_position);
			Matrix scaleTransform = Matrix::Scale(_scale);

			return positionTransform * rotationTransform * scaleTransform;
		}
	}
}
