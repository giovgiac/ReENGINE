/*
 * DirectionalLight.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Entities/Light.hpp"

namespace Re
{
    namespace Entities
    {
        class DirectionalLight : public Light
        {
        public:
            DirectionalLight();
            explicit DirectionalLight(Math::Color lightColor, Math::Vector3 lightDirection, f32 ambientStrength, f32 diffuseStrength);

            Math::Vector3 GetDirection() const;

        private:
            Math::Vector3 _direction;

        };
    }
}
