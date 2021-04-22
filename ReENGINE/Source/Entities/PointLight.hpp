/*
 * PointLight.hpp
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
        class PointLight : public Light
        {
        public:
            PointLight();
            explicit PointLight(Math::Color lightColor, Math::Vector3 position, f32 ambientStrength, f32 diffuseStrength, 
                f32 linear, f32 quadratic);

            Math::Vector3 GetPosition() const;
            f32 GetConstantAttenuation() const;
            f32 GetLinearAttenuation() const;
            f32 GetQuadraticAttenuation() const;

            void SetPosition(const Math::Vector3& position);
            void SetLinearAttenuation(f32 attenuation);
            void SetQuadraticAttenuation(f32 attenuation);
            
        private:
            Math::Vector3 _position;

            f32 _constantAttenuation;
            f32 _linearAttenuation;
            f32 _quadraticAttenuation;

        };
    }
}
