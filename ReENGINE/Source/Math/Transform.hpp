/**
 * Transform.h
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Rotator.hpp"
#include "Matrix.hpp"
#include "Vector3.hpp"

namespace Re
{
	namespace Math
	{
		struct Transform
		{
			Rotator _rotation;
			Vector3 _position;
			Vector3 _scale;

		public:
			Vector3 Forward() const;
			Vector3 Right() const;
			Vector3 Up() const;

			Matrix ToModel() const;

		};
	}
}
