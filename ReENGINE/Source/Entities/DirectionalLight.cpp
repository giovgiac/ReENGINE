/*
 * DirectionalLight.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "DirectionalLight.hpp"

namespace Re
{
    namespace Entities
    {
        DirectionalLight::DirectionalLight()
            : Light(), _direction(0.0f, 1.0f, 0.0f)
        {}

        DirectionalLight::DirectionalLight(Math::Color lightColor, Math::Vector3 lightDirection, f32 ambientStrength, f32 diffuseStrength)
            : Light(lightColor, ambientStrength, diffuseStrength)
        {
            // Set values as specified.
            _direction = lightDirection;
        }

        Math::Vector3 DirectionalLight::GetDirection() const
        {
            return _direction;
        }
    }
}
