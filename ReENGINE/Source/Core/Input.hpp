/**
 * Input.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

namespace Re
{
    namespace Core
    {
        namespace Input {

            enum class Action {
                Press,
                Release
            };

            enum class Keys {
                A,
                D,
                S,
                W
            };

        }
    }
}
