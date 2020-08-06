/*
 * RenderComponent.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "RenderComponent.hpp"

#include "Core/Entity.hpp"
#include "Core/World.hpp"

namespace Re
{
	namespace Components
	{
		RenderComponent::RenderComponent(boost::container::vector<Graphics::Vertex>& vertices)
			: _vertices(vertices)
		{}

		void RenderComponent::Initialize()
		{

		}

		void RenderComponent::Update(f32 DeltaTime)
		{}
	}
}
