/*
 * Entity.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Component.hpp"
#include "Graphics/Vertex.hpp"

#include <boost/container/map.hpp>
#include <boost/container/vector.hpp>
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
            virtual ~Entity();

            virtual void Initialize();
            virtual void Update(float DeltaTime);

            INLINE u32 GetId() const { return _id; }
            INLINE World* GetWorld() const { return _owner; }

            template <typename ComponentType, typename... ComponentArgs>
            ComponentType* AddComponent(ComponentArgs&&... args)
            {
                static_assert(boost::is_base_of<Component, ComponentType>::value,
                    "ComponentType passed for AddComponent does not inherit from Component.");
                auto newComponent = new ComponentType(std::forward<ComponentArgs>(args)...);

                _components.emplace(&typeid(ComponentType), newComponent);
                newComponent->Owner = this;
                newComponent->Initialize();
                return newComponent;
            }

            template <typename ComponentType>
            boost::container::vector<ComponentType*> GetComponents()
            {
                usize i = 0;
                boost::container::vector<ComponentType*> components(_components.count(&typeid(ComponentType)));
                for (auto it = _components.lower_bound(&typeid(ComponentType)); it != _components.upper_bound(&typeid(ComponentType)); ++it)
                    components[i++] = static_cast<ComponentType*>(it->second);

                return components;
            }

            template <typename ComponentType>
            bool HasComponent() const
            {
                return _components.contains(&typeid(ComponentType));
            }

            template <typename ComponentType>
            ComponentType* GetComponent()
            {
                return static_cast<ComponentType*>(_components.lower_bound(&typeid(ComponentType))->second);
            }

        protected:
            Entity();

        private:
            boost::container::multimap<const std::type_info*, Component*> _components;
            u32 _id;
            World* _owner;
        };
    }
}
