/*
 * Cube.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Cube.hpp"

#include "Graphics/Vertex.hpp"
#include "Math/Vector3.hpp"

#include <boost/container/vector.hpp>

namespace Re
{
	namespace Entities
	{
		Cube::Cube(const boost::shared_ptr<Graphics::Material>& material)
			: Entity(), _renderComponent(nullptr), _transformComponent(nullptr)
		{
			static boost::container::vector<Graphics::Vertex> vertices = {
				{ -0.5f, -0.5f, +0.5f,		+1.0f, +0.0f, +0.0f,		0.0f, 0.0f, 0.0f },
				{ +0.5f, -0.5f, +0.5f,		+1.0f, +0.0f, +0.0f,		0.0f, 0.0f, 0.0f },
				{ +0.5f, +0.5f, +0.5f,		+1.0f, +0.0f, +0.0f,		0.0f, 0.0f, 0.0f },
				{ -0.5f, +0.5f, +0.5f,		+1.0f, +0.0f, +0.0f,		0.0f, 0.0f, 0.0f },
				{ -0.5f, -0.5f, -0.5f,		+0.0f, +0.0f, +1.0f,		0.0f, 0.0f, 0.0f },
				{ +0.5f, -0.5f, -0.5f,		+0.0f, +0.0f, +1.0f,		0.0f, 0.0f, 0.0f },
				{ +0.5f, +0.5f, -0.5f,		+0.0f, +0.0f, +1.0f,		0.0f, 0.0f, 0.0f },
				{ -0.5f, +0.5f, -0.5f,		+0.0f, +0.0f, +1.0f,		0.0f, 0.0f, 0.0f },
			};

			static boost::container::vector<u32> indices = {
				0, 1, 3, 3, 1, 2,
				1, 5, 2, 2, 5, 6,
				5, 4, 6, 6, 4, 7,
				4, 0, 7, 7, 0, 3,
				3, 2, 7, 7, 2, 6,
				4, 5, 0, 0, 5, 1
			};

			// Calculate the normals using the average method.
			Graphics::CalculateAverageNormals(indices, vertices);

			// Create the default components for the Cube entity.
			_renderComponent = AddComponent<Components::RenderComponent>(vertices, indices, material);
			_transformComponent = AddComponent<Components::TransformComponent>();
		}

		Cube::Cube(f32 x, f32 y, f32 z, f32 scale, const boost::shared_ptr<Graphics::Material>& material)
			: Cube(material)
		{
			// Set position and scale values.
			_transformComponent->SetPosition(x, y, z);
			_transformComponent->SetScale(scale, scale, scale);
		}

		void Cube::Initialize()
		{
			Entity::Initialize();
		}

		void Cube::Update(f32 deltaTime)
		{
			Entity::Update(deltaTime);

			// Set a smooth rotation on the cube object (for testing purposes).
			static f32 rotationSpeed = 32.0f;
			Math::Rotator cubeRotation = _transformComponent->GetRotation();
			if (cubeRotation._yaw >= 360.0f)
				_transformComponent->SetRotation(cubeRotation._pitch, cubeRotation._roll, 0.0f);
			//_transformComponent->Rotate(0.0f, 0.0f, rotationSpeed * deltaTime);
		}
	}
}
