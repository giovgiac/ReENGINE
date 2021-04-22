/*
 * TransformComponent.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "TransformComponent.hpp"

#include <boost/bind.hpp>

namespace Re
{
	namespace Components
	{
		TransformComponent::TransformComponent()
			: _transform(Math::Transform()), _model(Math::Matrix::Identity())
		{
			// Set default scale values.
			_transform._scale.X = 1.0f;
			_transform._scale.Y = 1.0f;
			_transform._scale.Z = 1.0f;

			// Bind transform changed event locally.
			OnTransformChanged.connect(boost::bind(&TransformComponent::TransformChanged, this));

			// Announce transform changed.
			OnTransformChanged();
		}

		TransformComponent::TransformComponent(f32 x, f32 y, f32 z)
			: TransformComponent()
		{
			// Set specified position values.
			_transform._position.X = x;
			_transform._position.Y = y;
			_transform._position.Z = z;

			// Announce transform changed.
			OnTransformChanged();
		}

		TransformComponent::TransformComponent(f32 x, f32 y, f32 z, f32 pitch, f32 roll, f32 yaw)
			: TransformComponent(x, y, z)
		{
			// Set specified rotation values.
			_transform._rotation._pitch = pitch;
			_transform._rotation._roll = roll;
			_transform._rotation._yaw = yaw;

			// Announce transform changed.
			OnTransformChanged();
		}

		TransformComponent::TransformComponent(f32 x, f32 y, f32 z, f32 pitch, f32 roll, f32 yaw, f32 scale)
			: TransformComponent(x, y, z, pitch, roll, yaw)
		{
			// Set specified scale values.
			_transform._scale.X = scale;
			_transform._scale.Y = scale;
			_transform._scale.Z = scale;

			// Announce transform changed.
			OnTransformChanged();
		}

		TransformComponent::~TransformComponent()
		{
			// Clear listening events upon component destruction.
			OnTransformChanged.disconnect_all_slots();
		}

		void TransformComponent::Initialize()
		{
			Core::Debug::Log(NTEXT("TransformComponent initialized!\n"));
		}

		void TransformComponent::Update(f32 deltaTime)
		{}

		Math::Vector3 TransformComponent::GetPosition() const
		{
			return _transform._position;
		}

		Math::Rotator TransformComponent::GetRotation() const
		{
			return _transform._rotation;
		}

		Math::Vector3 TransformComponent::GetScale() const
		{
			return _transform._scale;
		}

		Math::Matrix TransformComponent::GetModel() const
		{
			return _model;
		}

		void TransformComponent::SetPosition(f32 newX, f32 newY, f32 newZ)
		{
			_transform._position.X = newX;
			_transform._position.Y = newY;
			_transform._position.Z = newZ;

			// Announce transform changed.
			OnTransformChanged();
		}

		void TransformComponent::SetRotation(f32 newPitch, f32 newRoll, f32 newYaw)
		{
			_transform._rotation._pitch = newPitch;
			_transform._rotation._roll = newRoll;
			_transform._rotation._yaw = newYaw;

			// Announce transform changed.
			OnTransformChanged();
		}

		void TransformComponent::SetScale(f32 newX, f32 newY, f32 newZ)
		{
			_transform._scale.X = newX;
			_transform._scale.Y = newY;
			_transform._scale.Z = newZ;

			// Announce transform changed.
			OnTransformChanged();
		}

		void TransformComponent::Translate(f32 dx, f32 dy, f32 dz)
		{
			_transform._position.X += dx;
			_transform._position.Y += dy;
			_transform._position.Z += dz;

			// Announce transform changed.
			OnTransformChanged();
		}

		void TransformComponent::Rotate(f32 dp, f32 dr, f32 dy)
		{
			_transform._rotation._pitch += dp;
			_transform._rotation._roll += dr;
			_transform._rotation._yaw += dy;

			// Announce transform changed.
			OnTransformChanged();
		}

		void TransformComponent::Scale(f32 f)
		{
			_transform._scale *= f;

			// Announce transform changed.
			OnTransformChanged();
		}

		void TransformComponent::Scale(f32 fx, f32 fy, f32 fz)
		{
			_transform._scale.X *= fx;
			_transform._scale.Y *= fy;
			_transform._scale.Z *= fz;

			// Announce transform changed.
			OnTransformChanged();
		}

		void TransformComponent::TransformChanged()
		{
			// Update model matrix.
			_model = _transform.ToModel();
		}

		const Math::Transform& TransformComponent::GetTransform() const
		{
			return _transform;
		}
	}
}
