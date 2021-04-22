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
		RenderComponent::RenderComponent(boost::container::vector<Graphics::Vertex>& vertices, boost::container::vector<u32>& indices, const boost::shared_ptr<Graphics::Material>& material)
			: _vertices(vertices), _indices(indices), _material(material)
		{}

		void RenderComponent::Initialize()
		{
			Core::Debug::Log(NTEXT("RenderComponent initialized!\n"));
		}

		void RenderComponent::Update(f32 deltaTime)
		{}

		boost::container::vector<u32> RenderComponent::GetIndices() const
		{
			return _indices;
		}

		boost::container::vector<Graphics::Vertex> RenderComponent::GetVertices() const
		{
			return _vertices;
		}

		Graphics::Material* RenderComponent::GetMaterial() const
		{
			return _material.get();
		}
	}
}
