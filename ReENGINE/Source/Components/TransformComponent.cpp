/*
 * TransformComponent.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "TransformComponent.hpp"

Re::Components::TransformComponent::TransformComponent(f32 InX, f32 InY, f32 InZ)
	: _position(InX, InY, InZ)
{}

void Re::Components::TransformComponent::Initialize()
{
	Core::Debug::Log(NTEXT("TransformComponent initialized!\n"));
}

void Re::Components::TransformComponent::Update(f32 DeltaTime)
{

}
