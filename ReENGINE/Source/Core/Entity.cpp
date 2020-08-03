/*
 * Entity.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Entity.hpp"

static int entityId = 0;

namespace Re
{
	namespace Core
	{
		Entity::Entity()
			: _id(entityId++) {}

		void Entity::Initialize()
		{
			//Debug::Log(NTEXT("Entity initialized!\n"));
		}

		void Entity::Update(float DeltaTime)
		{
			for (const auto& component : _components)
			{
				component.second->Update(DeltaTime);
			}
		}
	}
}
