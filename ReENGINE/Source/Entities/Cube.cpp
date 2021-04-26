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
			static boost::container::vector<Graphics::Vertex> vertices = 
			{
				// Back Face
				{ -0.5f, -0.5f, +0.5f,		+0.0f, +0.0f, +1.0f,		+0.0f, +0.0f },		//  0
				{ +0.5f, -0.5f, +0.5f,		+0.0f, +0.0f, +1.0f,		+1.0f, +0.0f },		//  1
				{ -0.5f, +0.5f, +0.5f,		+0.0f, +0.0f, +1.0f,		+0.0f, +1.0f },		//  2
				{ +0.5f, +0.5f, +0.5f,		+0.0f, +0.0f, +1.0f,		+1.0f, +1.0f },		//  3
																						    
				// Right Face															    
				{ +0.5f, -0.5f, +0.5f,		+1.0f, +0.0f, +0.0f,		+1.0f, +0.0f },		//  4
				{ +0.5f, -0.5f, -0.5f,		+1.0f, +0.0f, +0.0f,		+0.0f, +0.0f },		//  5
				{ +0.5f, +0.5f, +0.5f,		+1.0f, +0.0f, +0.0f,		+1.0f, +1.0f },		//  6
				{ +0.5f, +0.5f, -0.5f,		+1.0f, +0.0f, +0.0f,		+0.0f, +1.0f },		//  7
																					    
				// Front Face											   
				{ -0.5f, -0.5f, -0.5f,		+0.0f, +0.0f, -1.0f,		+0.0f, +1.0f },		//  8
				{ +0.5f, -0.5f, -0.5f,		+0.0f, +0.0f, -1.0f,		+1.0f, +1.0f },		//  9
				{ -0.5f, +0.5f, -0.5f,		+0.0f, +0.0f, -1.0f,		+0.0f, +0.0f },		// 10
				{ +0.5f, +0.5f, -0.5f,		+0.0f, +0.0f, -1.0f,		+1.0f, +0.0f },		// 11
																	   
				// Left Face											   
				{ -0.5f, -0.5f, -0.5f,		-1.0f, +0.0f, +0.0f,		+1.0f, +0.0f },		// 12
				{ -0.5f, -0.5f, +0.5f,		-1.0f, +0.0f, +0.0f,		+0.0f, +0.0f },		// 13
				{ -0.5f, +0.5f, -0.5f,		-1.0f, +0.0f, +0.0f,		+1.0f, +1.0f },		// 14
				{ -0.5f, +0.5f, +0.5f,		-1.0f, +0.0f, +0.0f,		+0.0f, +1.0f },		// 15
																		   
				// Up Face												   
				{ -0.5f, +0.5f, +0.5f,		+0.0f, +1.0f, +0.0f,		+0.0f, +1.0f },		// 16
				{ +0.5f, +0.5f, +0.5f,		+0.0f, +1.0f, +0.0f,		+1.0f, +1.0f },		// 17
				{ -0.5f, +0.5f, -0.5f,		+0.0f, +1.0f, +0.0f,		+0.0f, +0.0f },		// 18
				{ +0.5f, +0.5f, -0.5f,		+0.0f, +1.0f, +0.0f,		+1.0f, +0.0f },		// 19
																	   
				// Down Face											   
				{ -0.5f, -0.5f, -0.5f,		+0.0f, -1.0f, +0.0f,		+0.0f, +1.0f },		// 20
				{ +0.5f, -0.5f, -0.5f,		+0.0f, -1.0f, +0.0f,		+1.0f, +1.0f },		// 21
				{ -0.5f, -0.5f, +0.5f,		+0.0f, -1.0f, +0.0f,		+0.0f, +0.0f },		// 22
				{ +0.5f, -0.5f, +0.5f,		+0.0f, -1.0f, +0.0f,		+1.0f, +0.0f },		// 23
			};

			static boost::container::vector<u32> indices =
			{
				  0,  1,  2,  2,  1,  3,													// Back
				  4,  5,  6,  6,  5,  7,													// Right
				  9,  8, 11, 11,  8, 10,													// Front
				 12, 13, 14, 14, 13, 15,													// Left
				 16, 17, 18, 18, 17, 19,													// Up
				 20, 21, 22, 22, 21, 23														// Down
			};

			// Calculate the normals using the average method.
			// Graphics::CalculateAverageNormals(vertices, indices);

			// Create the default components for the Cube entity.
			if (material)
				_renderComponent = AddComponent<Components::RenderComponent>(vertices, indices, material);
			else
				_renderComponent = AddComponent<Components::RenderComponent>(vertices, indices);
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

			// TEST CODE: Set a smooth rotation on the cube object (for testing purposes).
			static f32 rotationSpeed = 32.0f;
			Math::Rotator rotation = _transformComponent->GetRotation();
			if (rotation._yaw >= 360.0f)
				_transformComponent->SetRotation(rotation._pitch, rotation._roll, 0.0f);
			//_transformComponent->Rotate(0.0f, 0.0f, rotationSpeed * deltaTime);
		}
	}
}
