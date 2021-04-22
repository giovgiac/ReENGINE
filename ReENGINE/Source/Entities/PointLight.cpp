/*
 * PointLight.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "PointLight.hpp"

namespace Re
{
    namespace Entities
    {
        PointLight::PointLight()
            : Light(), _position(0.0f), _constantAttenuation(1.0f), _linearAttenuation(0.0f), _quadraticAttenuation(0.0f)
        {}

        PointLight::PointLight(Math::Color lightColor, Math::Vector3 position, f32 ambientStrength, f32 diffuseStrength, f32 linear, f32 quadratic)
            : Light(lightColor, ambientStrength, diffuseStrength)
        {
            // Set values as specified.
            _position = position;
            _constantAttenuation = 1.0f;
            _linearAttenuation = linear;
            _quadraticAttenuation = quadratic;
        }

        Math::Vector3 PointLight::GetPosition() const
        {
            return _position;
        }

        f32 PointLight::GetConstantAttenuation() const
        {
            return _constantAttenuation;
        }

        f32 PointLight::GetLinearAttenuation() const
        {
            return _linearAttenuation;
        }

        f32 PointLight::GetQuadraticAttenuation() const
        {
            return _quadraticAttenuation;
        }

        void PointLight::SetPosition(const Math::Vector3& position)
        {
            _position = position;
            OnParameterChanged();
        }

        void PointLight::SetLinearAttenuation(f32 attenuation)
        {
            _linearAttenuation = attenuation;
            OnParameterChanged();
        }

        void PointLight::SetQuadraticAttenuation(f32 attenuation)
        {
            _quadraticAttenuation = attenuation;
            OnParameterChanged();
        }
    }
}
