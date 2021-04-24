/*
 * Material.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Material.hpp"

namespace Re
{
    namespace Graphics
    {
        Material::Material(const boost::shared_ptr<Texture>& texture)
            : _specularPower(1.0f), _specularStrength(0.0f)
        {
            static boost::shared_ptr<Texture> defaultTexture = boost::make_shared<Texture>();

            // Assign texture or provide default texture.
            if (texture)
                _texture = texture;
            else
                _texture = defaultTexture;
        }

        Material::Material(f32 specularPower, f32 specularStrength, const boost::shared_ptr<Texture>& texture)
            : Material(texture)
        {
            // Assign specified values
            _specularPower = specularPower;
            _specularStrength = specularStrength;
        }

        f32 Material::GetSpecularPower() const
        {
            return _specularPower;
        }

        f32 Material::GetSpecularStrength() const
        {
            return _specularStrength;
        }

        Texture* Material::GetTexture() const
        {
            return _texture.get();
        }
    }
}
