/*
 * RenderComponent.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"
#include "Core/Component.hpp"
#include "Graphics/Vertex.hpp"

#include <boost/container/vector.hpp>

namespace Re 
{
    namespace Components
    {
        class RenderComponent : public Core::Component
        {
        public:
            explicit RenderComponent(boost::container::vector<Graphics::Vertex>& vertices);

            virtual void Initialize() override;
            virtual void Update(f32 DeltaTime) override;

            INLINE boost::container::vector<Graphics::Vertex> GetVertices() const { return _vertices; }

        private:
            boost::container::vector<Graphics::Vertex> _vertices;

        };
    }
}
