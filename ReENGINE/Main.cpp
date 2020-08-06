/*
 * Main.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Core/GameManager.hpp"
#include "Core/NewtonManager.hpp"
#include "Localization/Language.hpp"

#include "Core/Entity.hpp"
#include "Components/RenderComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Memory/StackAllocator.hpp"

#include "Core/World.hpp"
#include "Platform/Win32/Timer.hpp"

namespace boost
{
	void throw_exception(std::exception const& e)
	{
		Re::Core::Debug::Log(NTEXT("Boost exception: %s\n"), e.what());
	}
}

using namespace Re::Core;
using namespace Re::Core::Localization;

// Game Test Class
class TestGame : public GameManager 
{
public:
	TestGame() {}
	~TestGame() {}
	
	void Draw(const Re::Platform::ITimer&) override {}
	void OnKeyUp(WPARAM wParam) override 
	{
		if (wParam == VK_ESCAPE)
			PostQuitMessage(0);
	}

	NRESULT StartUp() override 
	{
		return NSUCCESS;
	}

	NRESULT ShutDown() override 
	{
		return NSUCCESS;
	}

	void Update(const Re::Platform::ITimer&) override {}
};

// Newton Manager
NewtonManager nNewtonManager;
TestGame nGameManager;

boost::container::vector<Re::Graphics::Vertex> vertices = {
	{ +0.4f, -0.4f, +0.0f,		+1.0f, +0.0f, +0.0f },
	{ +0.4f, +0.4f, +0.0f,		+0.0f, +1.0f, +0.0f },
	{ -0.4f, +0.4f, +0.0f,		+0.0f, +0.0f, +1.0f },

	{ -0.4f, +0.4f, +0.0f,		+0.0f, +0.0f, +1.0f },
	{ -0.4f, -0.4f, +0.0f,		+1.0f, +1.0f, +0.0f },
	{ +0.4f, -0.4f, +0.0f,		+1.0f, +0.0f, +0.0f },
};

#define NUM_ENTITIES 1

/* TESTING CLASS - TO BE FULLY IMPLEMENTED LATER */
// WINDOWS MAIN: i32 WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, i32) 
int main()
{
	// Memory Debugging
	#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif

	World world;
	
	for (usize i = 0; i < NUM_ENTITIES; ++i)
	{
		auto ent = world.SpawnEntity<Entity>();
		ent->AddComponent<Re::Components::RenderComponent>(vertices);
	}

	world.Startup();
	world.Loop();
	world.Shutdown();

	// Module Start-up
	nNewtonManager.StartUp();
	
	// Start Game
	nGameManager.StartUp();
	
	// Stop Game
	nGameManager.ShutDown();

	// Module Shut-down
	nNewtonManager.ShutDown();

	// Exit the Program
	return EXIT_SUCCESS;
}
