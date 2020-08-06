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
			virtual void Initialize() = 0;
			virtual void Update(f32 DeltaTime) = 0;

		protected:
			INLINE Entity* GetOwner() const { return _owner; }

		private:
			Entity* _owner;

		};
	}
}
