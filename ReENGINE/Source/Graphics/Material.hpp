/*
 * Material.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Graphics/Texture.hpp"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/signals2.hpp>

namespace Re
{
    namespace Graphics
    {
        class Material
        {
        public:
            Material(const boost::shared_ptr<Texture>& texture = nullptr);
            explicit Material(f32 specularPower, f32 specularStrength, const boost::shared_ptr<Texture>& texture = nullptr);

            f32 GetSpecularPower() const;
            f32 GetSpecularStrength() const;
            Texture* GetTexture() const;

        public:
            boost::signals2::signal<void()> OnParameterChanged;
            
        private:
            f32 _specularPower;
            f32 _specularStrength;

            boost::shared_ptr<Texture> _texture;

        };
    }
}
