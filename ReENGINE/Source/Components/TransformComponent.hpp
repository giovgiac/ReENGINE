/*
 * TransformComponent.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Component.hpp"
#include "Math/Transform.hpp"

#include <boost/signals2.hpp>

namespace Re
{
	namespace Components
	{
		class TransformComponent : public Core::Component
		{
		public:
			TransformComponent();
			explicit TransformComponent(f32 x, f32 y, f32 z);
			explicit TransformComponent(f32 x, f32 y, f32 z, f32 pitch, f32 roll, f32 yaw);
			explicit TransformComponent(f32 x, f32 y, f32 z, f32 pitch, f32 roll, f32 yaw, f32 scale);
			virtual ~TransformComponent();

			virtual void Initialize() override;
			virtual void Update(f32 deltaTime) override;

			Math::Vector3 GetPosition() const;
			Math::Rotator GetRotation() const;
			Math::Vector3 GetScale() const;
			Math::Matrix GetModel() const;

			void SetPosition(f32 newX, f32 newY, f32 newZ);
			void SetRotation(f32 newPitch, f32 newRoll, f32 newYaw);
			void SetScale(f32 newX, f32 newY, f32 newZ);

			void Translate(f32 dx, f32 dy, f32 dz);
			void Rotate(f32 dp, f32 dr, f32 dy);
			void Scale(f32 f);
			void Scale(f32 fx, f32 fy, f32 fz);

			void TransformChanged();

			const Math::Transform& GetTransform() const;

		public:
			boost::signals2::signal<void()> OnTransformChanged;

		private:
			Math::Matrix _model;
			Math::Transform _transform;

		};
	}
}
