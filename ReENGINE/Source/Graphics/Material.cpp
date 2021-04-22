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
        Material::Material()
            : _specularPower(1.0f), _specularStrength(0.0f)
        {}

        Material::Material(f32 specularPower, f32 specularStrength)
            : _specularPower(specularPower), _specularStrength(specularStrength)
        {}

        f32 Material::GetSpecularPower() const
        {
            return _specularPower;
        }

        f32 Material::GetSpecularStrength() const
        {
            return _specularStrength;
        }
    }
}
