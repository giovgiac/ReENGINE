/*
 * Light.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Entity.hpp"
#include "Math/Color.hpp"
#include "Math/Vector3.hpp"

#include <boost/signals2.hpp>

namespace Re
{
    namespace Entities
    {
        class Light : public Core::Entity
        {
        protected:
            Light();
            explicit Light(Math::Color lightColor, f32 ambientStrength, f32 diffuseStrength);
            virtual ~Light();

        public:
            Math::Vector3 GetColor() const;
            f32 GetAmbientStrength() const;
            f32 GetDiffuseStrength() const;

        public:
            boost::signals2::signal<void()> OnParameterChanged;

        private:
            Math::Color _color;

            f32 _ambientStrength;
            f32 _diffuseStrength;
        };
    }
}
