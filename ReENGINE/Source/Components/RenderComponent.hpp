/*
 * RenderComponent.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Component.hpp"
#include "Graphics/Material.hpp"
#include "Graphics/Vertex.hpp"

#include <boost/make_shared.hpp>
#include <boost/container/vector.hpp>

namespace Re 
{
    namespace Components
    {
        class RenderComponent : public Core::Component
        {
        public:
            explicit RenderComponent(boost::container::vector<Graphics::Vertex>& vertices, boost::container::vector<u32>& indices, const boost::shared_ptr<Graphics::Material>& material = boost::make_shared<Graphics::Material>());

            virtual void Initialize() override;
            virtual void Update(f32 deltaTime) override;

            boost::container::vector<u32> GetIndices() const;
            boost::container::vector<Graphics::Vertex> GetVertices() const;
            Graphics::Material* GetMaterial() const;

        private:
            boost::container::vector<Graphics::Vertex> _vertices;
            boost::container::vector<u32> _indices;
            boost::shared_ptr<Graphics::Material> _material;

        };
    }
}
