/*
 * Entity.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"
#include "Component.hpp"

#include <boost/container/map.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/type_traits/is_base_of.hpp>

namespace Re
{
	namespace Core
	{
		class Entity
		{
			friend class World;

			public:
				Entity();

				virtual void Initialize();
				virtual void Update(float DeltaTime);

				INLINE u32 GetId() const { return _id; }

				template <typename ComponentType, typename... ComponentArgs>
				Component& AddComponent(ComponentArgs&&... args)
				{
					static_assert(boost::is_base_of<Component, ComponentType>::value, "ComponentType passed for AddComponent does not inherit from Component.");

					auto newComponent = boost::make_shared<ComponentType>(std::forward<ComponentArgs>(args)...);
					_components.emplace_unique(&typeid(ComponentType), newComponent);
					newComponent->_owner = this;
					newComponent->Initialize();
					return *newComponent;
				}

				template <typename ComponentType>
				bool HasComponent() const
				{
					return _components.find(&typeid(ComponentType)) != _components.end();
				}

				template <typename ComponentType>
				boost::weak_ptr<Component> GetComponent()
				{
					if (HasComponent<ComponentType>())
					{
						return static_cast<boost::weak_ptr<Component>>(_components[&typeid(ComponentType)]);
					}

					return boost::weak_ptr<Component>();
				}

		private:
			boost::container::map<const std::type_info*, boost::shared_ptr<Component>> _components;
			u32 _id;
			World* _owner;

		};
	}
}
