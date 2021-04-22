/*
 * Entity.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"
#include "Graphics/Vertex.hpp"
#include "Component.hpp"

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

                _components.emplace_unique(typeid(ComponentType), newComponent);
                newComponent->Owner = this;
                newComponent->Initialize();
                return newComponent;
            }

            template <typename ComponentType>
            bool HasComponent() const
            {
                return _components.contains(typeid(ComponentType));
            }

            template <typename ComponentType>
            ComponentType* GetComponent()
            {
                return static_cast<ComponentType*>(_components[typeid(ComponentType)]);
            }

        protected:
            Entity();

        private:
            boost::container::map<std::type_index, Component*> _components;
            u32 _id;
            World* _owner;
        };
    }
}
