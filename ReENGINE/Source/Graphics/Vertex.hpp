/*
 * Vertex.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Math/Color.hpp"
#include "Math/Vector.hpp"
#include "Math/Vector3.hpp"

#include <boost/container/vector.hpp>

namespace Re
{
    namespace Graphics
    {
        struct Vertex
        {
            alignas(16) Math::Vector3 _position;
            alignas(16) Math::Vector3 _normal;
            alignas(8)  Math::Vector  _textureCoordinate;

            Vertex()
                : _position(0.0f), _normal(0.0f), _textureCoordinate(0.0f) {}

            Vertex(f32 InX, f32 InY, f32 InZ, f32 InNX, f32 InNY, f32 InNZ, f32 InU, f32 InV)
                : _position(InX, InY, InZ), _normal(InNX, InNY, InNZ), _textureCoordinate(InU, InV) {}
        };

        // Utility functions for vertex manipulation.
        void CalculateAverageNormals(boost::container::vector<Vertex>& vertices, const boost::container::vector<u32>& indices);
    }
}
