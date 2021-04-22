/*
 * SpotLight.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Entities/PointLight.hpp"

namespace Re
{
    namespace Entities
    {
        class SpotLight : public PointLight
        {
        public:
            SpotLight();
            explicit SpotLight(Math::Color lightColor, Math::Vector3 position, Math::Vector3 direction, f32 cutoffAngle, 
                f32 ambientStrength, f32 diffuseStrength, f32 linear, f32 quadratic);

            Math::Vector3 GetDirection() const;
            f32 GetCutoffAngle() const;

            void SetDirection(const Math::Vector3& direction);
            void SetCutoffAngle(f32 cutoffAngle);

        private:
            Math::Vector3 _direction;

            f32 _cutoffAngle;
        };
    }
}
