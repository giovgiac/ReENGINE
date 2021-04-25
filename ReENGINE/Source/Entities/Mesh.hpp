/*
 * Mesh.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Components/TransformComponent.hpp"
#include "Core/Entity.hpp"

namespace Re
{
    namespace Entities
    {
        class Mesh : public Core::Entity
        {
        public:
            Mesh();
            explicit Mesh(const utf8* filename);

            virtual void Initialize() override;
            virtual void Update(f32 deltaTime) override;

            void Load();

            Components::TransformComponent* GetTransform() const;

        private:
            const utf8* _filename;

            Components::TransformComponent* _transformComponent;

        };
    }
}
