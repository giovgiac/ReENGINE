/*
 * Camera.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Components/InputComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Core/Entity.hpp"
#include "Core/Input.hpp"
#include "Math/Matrix.hpp"

#include <boost/container/map.hpp>
#include <boost/signals2.hpp>

namespace Re
{
	namespace Entities
	{
		class Camera : public Core::Entity
		{
		public:
			Camera();
			explicit Camera(f32 fov, f32 nearZ, f32 farZ);
			virtual ~Camera();

			virtual void Initialize() override;
			virtual void Update(f32 deltaTime) override;

			void HandleInput(f32 deltaTime);
			void HandleKeyboardInput(f32 deltaTime);
			void HandleMouseInput();

			Components::TransformComponent* GetTransform() const;
			Math::Matrix GetProjection(f32 aspectRatio) const;
			Math::Matrix GetView() const;

		private:
			void UpdateView();

		public:
			boost::signals2::signal<void()> OnParameterChanged;

		private:
			Components::InputComponent* _inputComponent;
			Components::TransformComponent* _transformComponent;

			f32 _fieldOfView;
			f32 _nearZ;
			f32 _farZ;

			Math::Matrix _cameraView;
		};
	}
}
