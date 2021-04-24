/*
 * Cube.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Components/RenderComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Core/Entity.hpp"

namespace Re
{
	namespace Entities
	{
		class Cube : public Core::Entity
		{
		public:
			Cube(const boost::shared_ptr<Graphics::Material>& material = nullptr);
			explicit Cube(f32 x, f32 y, f32 z, f32 scale, const boost::shared_ptr<Graphics::Material>& material = nullptr);

			virtual void Initialize() override;
			virtual void Update(f32 deltaTime) override;

		private:
			Components::RenderComponent* _renderComponent;
			Components::TransformComponent* _transformComponent;
			
		};
	}
}
