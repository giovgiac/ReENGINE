/*
 * TransformComponent.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Component.hpp"
#include "Math/Vector3.hpp"

namespace Re
{
	namespace Components
	{
		class TransformComponent : public Core::Component
		{
		public:
			TransformComponent(f32 InX, f32 InY, f32 InZ);

			virtual void Initialize() override;
			virtual void Update(f32 DeltaTime) override;

		private:
			Math::NVector3 _position;

		};
	}
}
