/*
 * Light.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Light.hpp"

namespace Re
{
    namespace Entities
    {
        Light::Light()
            : Entity(), _color(Math::Colors::White), _ambientStrength(1.0f), _diffuseStrength(0.0f)
        {}

        Light::Light(Math::Color lightColor, f32 ambientStrength, f32 diffuseStrength)
            : Light()
        {
            // Update light values.
            _color = lightColor;
            _ambientStrength = ambientStrength;
            _diffuseStrength = diffuseStrength;
        }

        Light::~Light()
        {
            // Clear listening events upon light destruction.
            OnParameterChanged.disconnect_all_slots();
        }

        Math::Vector3 Light::GetColor() const
        {
            return Math::Vector3(_color.Red, _color.Green, _color.Blue);
        }

        f32 Light::GetAmbientStrength() const
        {
            return _ambientStrength;
        }

        f32 Light::GetDiffuseStrength() const
        {
            return _diffuseStrength;
        }

        void Light::SetColor(Math::Color lightColor)
        {
            _color = lightColor;
            OnParameterChanged();
        }

        void Light::SetAmbientStrength(f32 ambientStrength)
        {
            _ambientStrength = ambientStrength;
            OnParameterChanged();
        }

        void Light::SetDiffuseStrength(f32 diffuseStrength)
        {
            _diffuseStrength = diffuseStrength;
            OnParameterChanged();
        }
    }
}
