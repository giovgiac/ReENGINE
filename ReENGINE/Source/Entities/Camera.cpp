/*
 * Camera.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Camera.hpp"

#include "Core/World.hpp"

#include <boost/bind.hpp>

namespace Re
{
	namespace Entities
	{
		Camera::Camera()
			: Entity(), _inputComponent(nullptr), _transformComponent(nullptr), _fieldOfView(45.0f), _nearZ(0.1f), _farZ(100.0f)
		{
			_inputComponent = AddComponent<Components::InputComponent>();
			_transformComponent = AddComponent<Components::TransformComponent>();

			// Bind event when transform changes.
			_transformComponent->OnTransformChanged.connect(boost::bind(&Camera::UpdateView, this));
		}

		Camera::Camera(f32 fov, f32 nearZ, f32 farZ)
			: Camera()
		{
			// Fill in specified camera parameters.
			_fieldOfView = fov;
			_nearZ = nearZ;
			_farZ = farZ;
		}

		Camera::~Camera()
		{
			// Clear listening events from camera upon destruction.
			OnParameterChanged.disconnect_all_slots();
		}

		void Camera::Initialize()
		{
			Entity::Initialize();
		}

		void Camera::Update(f32 deltaTime)
		{
			Entity::Update(deltaTime);

			HandleInput(deltaTime);
		}

		void Camera::HandleInput(f32 deltaTime)
		{
			HandleKeyboardInput(deltaTime);
			HandleMouseInput();
		}

		void Camera::HandleKeyboardInput(f32 deltaTime)
		{
			static f32 moveSpeed = 8.0f;
			f32 deltaPosition = moveSpeed * deltaTime;
			Math::Vector3 translation = Math::Vector3(0.0f);

			if (_inputComponent->IsKeyDown(Core::Input::Keys::W))
			{
				translation += _transformComponent->GetTransform().Forward() * deltaPosition;
			}

			if (_inputComponent->IsKeyDown(Core::Input::Keys::S))
			{
				translation -= _transformComponent->GetTransform().Forward() * deltaPosition;
			}

			if (_inputComponent->IsKeyDown(Core::Input::Keys::D))
			{
				translation += _transformComponent->GetTransform().Right() * deltaPosition;
			}

			if (_inputComponent->IsKeyDown(Core::Input::Keys::A))
			{
				translation -= _transformComponent->GetTransform().Right() * deltaPosition;
			}

			if (translation != Math::Vector3(0.0f))
				_transformComponent->Translate(translation.X, translation.Y, translation.Z);
		}

		void Camera::HandleMouseInput()
		{
			static f32 pitchLimit = 70.0f;
			static f32 turnSpeed = 0.25f;

			// Request latest mouse displacement to input component.
			Math::Vector mouseDisplacement = _inputComponent->GetMouseDisplacement();

			// Update rotation of the camera per mouse change.
			if (mouseDisplacement != Math::Vector::Zero())
			{
				auto cameraRotation = _transformComponent->GetRotation();
				_transformComponent->SetRotation(
					Math::Clamp(cameraRotation._pitch + mouseDisplacement.Y * turnSpeed, -pitchLimit, +pitchLimit),
					cameraRotation._roll,
					cameraRotation._yaw + mouseDisplacement.X * turnSpeed
				);
			}
		}

		Components::TransformComponent* Camera::GetTransform() const
		{
			return _transformComponent;
		}

		Math::Matrix Camera::GetProjection(f32 aspectRatio) const
		{
			return Math::Matrix::Perspective(
				aspectRatio, _fieldOfView, _nearZ, _farZ
			);
		}

		Math::Matrix Camera::GetView() const
		{
			return _cameraView;
		}

		void Camera::UpdateView()
		{
			auto forward = _transformComponent->GetTransform().Forward();
			_cameraView = Math::Matrix::LookAt(
				_transformComponent->GetPosition(),
				_transformComponent->GetPosition() + _transformComponent->GetTransform().Forward(),
				_transformComponent->GetTransform().Up()
			);
		}
	}
}
