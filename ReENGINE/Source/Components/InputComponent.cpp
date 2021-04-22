/*
 * InputComponent.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "InputComponent.hpp"

#include "Core/Entity.hpp"
#include "Core/World.hpp"

#include <boost/bind.hpp>

namespace Re 
{
    namespace Components
    {
        InputComponent::InputComponent()
            : Component(), _mouseDisplacement(Math::Vector(0.0f))
        {
        }

        void InputComponent::Initialize()
        {
            auto owner = GetOwner();
            if (owner)
            {
                auto world = owner->GetWorld();
                if (world)
                {
                    auto& window = world->GetWindow();

                    // Bind functions to handle window input.
                    window.KeyEvent.connect(boost::bind(&InputComponent::KeyEvent, this, _1, _2));
                    window.MouseEvent.connect(boost::bind(&InputComponent::MouseEvent, this, _1, _2));
                }
            }            
            
            Core::Debug::Log(NTEXT("InputComponent initialized!\n"));
        }

        void InputComponent::Update(f32 deltaTime)
        {}

        bool InputComponent::IsKeyDown(Core::Input::Keys keyCode) const
        {
            if (_keyState.contains(keyCode))
                return _keyState.at(keyCode);
            return false;
        }

        Math::Vector InputComponent::GetMouseDisplacement() const
        {
            return _mouseDisplacement;
        }

        void InputComponent::KeyEvent(Core::Input::Action keyAction, Core::Input::Keys keyCode)
        {
            if (keyAction == Core::Input::Action::Press)
            {
                _keyState[keyCode] = true;
            }
            else
            {
                _keyState[keyCode] = false;
            }
        }

        void InputComponent::MouseEvent(i32 dx, i32 dy)
        {
            _mouseDisplacement.X = dx;
            _mouseDisplacement.Y = dy;
        }
    }
}
