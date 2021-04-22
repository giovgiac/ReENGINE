/*
 * Material.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

#include <boost/signals2.hpp>

namespace Re
{
    namespace Graphics
    {
        class Material
        {
        public:
            Material();
            explicit Material(f32 specularPower, f32 specularStrength);

            f32 GetSpecularPower() const;
            f32 GetSpecularStrength() const;

        public:
            boost::signals2::signal<void()> OnParameterChanged;
            
        private:
            f32 _specularPower;
            f32 _specularStrength;

        };
    }
}
