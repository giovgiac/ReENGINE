/*
 * Vertex.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Math/Color.hpp"
#include "Math/Vector3.hpp"

#include <boost/container/vector.hpp>

namespace Re
{
    namespace Graphics
    {
        struct Vertex
        {
            alignas(16) Math::Vector3 _position;
            alignas(16) Math::Vector3 _color;
            alignas(16) Math::Vector3 _normal;

            Vertex(f32 InX, f32 InY, f32 InZ, f32 InR, f32 InG, f32 InB, f32 InNX, f32 InNY, f32 InNZ)
                : _position(InX, InY, InZ), _color(InR, InG, InB), _normal(InNX, InNY, InNZ) {}
        };

        // Utility functions for vertex manipulation.
        void CalculateAverageNormals(const boost::container::vector<u32>& indices, boost::container::vector<Vertex>& vertices);
    }
}
