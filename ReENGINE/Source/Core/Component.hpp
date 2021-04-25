/*
 * Component.hpp
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
		class Component
		{
			friend class Entity;

		public:
			virtual ~Component() = default;
			virtual void Initialize() = 0;
			virtual void Update(f32 deltaTime) = 0;

		protected:
			INLINE Entity* GetEntity() const { return Owner; }

		private:
			Entity* Owner;

		};
	}
}
