/*
 * InputComponent.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Component.hpp"
#include "Core/Input.hpp"
#include "Math/Vector.hpp"

#include <boost/container/map.hpp>

namespace Re
{
	namespace Components
	{
		class InputComponent : public Core::Component
		{
		public:
			InputComponent();

			virtual void Initialize() override;
			virtual void Update(f32 deltaTime) override;

			bool IsKeyDown(Core::Input::Keys keyCode) const;
			Math::Vector GetMouseDisplacement() const;

			void KeyEvent(Core::Input::Action keyAction, Core::Input::Keys keyCode);
			void MouseEvent(i32 dx, i32 dy);
			
		private:
			boost::container::map<Core::Input::Keys, bool> _keyState;
			Math::Vector _mouseDisplacement;

		};
	}
}
