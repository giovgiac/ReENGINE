/*
 * SpotLight.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "SpotLight.hpp"

namespace Re
{
    namespace Entities
    {
        SpotLight::SpotLight()
            : PointLight(), _direction(0.0f, 0.0f, -1.0f), _cutoffAngle(0.0f)
        {}

        SpotLight::SpotLight(Math::Color lightColor, Math::Vector3 position, Math::Vector3 direction, f32 cutoffAngle, f32 ambientStrength, f32 diffuseStrength, f32 linear, f32 quadratic)
            : PointLight(lightColor, position, ambientStrength, diffuseStrength, linear, quadratic)
        {
            // Set values as specified.
            _direction = direction;
            _cutoffAngle = cutoffAngle;
        }

        Math::Vector3 SpotLight::GetDirection() const
        {
            return _direction;
        }

        f32 SpotLight::GetCutoffAngle() const
        {
            return _cutoffAngle;
        }

        void SpotLight::SetDirection(const Math::Vector3& direction)
        {
            _direction = direction;
            OnParameterChanged();
        }

        void SpotLight::SetCutoffAngle(f32 cutoffAngle)
        {
            _cutoffAngle = cutoffAngle;
            OnParameterChanged();
        }
    }
}
