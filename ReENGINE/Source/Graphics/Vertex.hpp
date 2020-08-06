/*
* Vertex.hpp
*
* Copyright (c) Giovanni Giacomo. All Rights Reserved.
*
*/

#pragma once

#include "Core/Debug/Assert.hpp"
#include "Math/Color.hpp"
#include "Math/Vector3.hpp"

namespace Re
{
    namespace Graphics
    {
        struct Vertex
        {
            Math::NVector3 _position;
            Math::NColor _color;

            Vertex(f32 InX, f32 InY, f32 InZ, f32 InR, f32 InG, f32 InB)
                : _position(InX, InY, InZ), _color(InR, InG, InB, 1.0f) {}
        };
    }
}
