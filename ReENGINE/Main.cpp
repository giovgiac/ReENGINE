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
#include "Entities/Cube.hpp"
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

using namespace Re;
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

const usize NUM_ENTITIES = 8192;

/* TESTING CLASS - TO BE FULLY IMPLEMENTED LATER */
// WINDOWS MAIN: i32 WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, i32) 
int main()
{
	// Memory Debugging
	#ifdef _DEBUG
	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
	#endif

	World world;

	// Initialize the world.
	world.Startup();

	{
		// TEST CODE: Create a camera to view the world from.
		auto camera = world.SpawnEntity<Entities::Camera>(45.0f, 0.1f, 1000.0f);
		camera->GetTransform()->Translate(3.0f, 0.0f, 7.0f);

		// TEST CODE: Create lights to illuminate the world.
		// auto directionalLight = SpawnEntity<Entities::DirectionalLight>(Math::Colors::White, Math::Vector3(2.0f, 1.0f, -2.0f), 0.2f, 0.7f);
		// auto pointLight0 = world.SpawnEntity<Entities::PointLight>(Math::Colors::White, Math::Vector3(3.0f, 2.0f, 3.0f), 0.10f, 1.0f, 0.2f, 0.3f);
		auto spotLight0 = world.SpawnEntity<Entities::SpotLight>(Math::Colors::White, Math::Vector3(3.0f, 0.0f, 7.0f), Math::Vector3(0.0f, 0.0f, -1.0f), 20.0f, 0.2f, 1.0f, 0.1f, 0.02f);
		camera->GetTransform()->OnTransformChanged.connect([camera, spotLight0]() {
			spotLight0->SetPosition(camera->GetTransform()->GetPosition());
			spotLight0->SetDirection(camera->GetTransform()->GetTransform().Forward());
		});

		// TEST CODE: Create some test textures.
		auto texture0 = boost::make_shared<Graphics::Texture>("Textures/brick.png");
		auto texture1 = boost::make_shared<Graphics::Texture>("Textures/dirt.png");

		// TEST CODE: Create some test materials.
		auto material0 = boost::make_shared<Graphics::Material>(32.0f, 1.0f, texture0);
		auto material1 = boost::make_shared<Graphics::Material>(1.0f, 0.0f);

		// TEST CODE: Create some test cubes.
		//auto cube0 = world.SpawnEntity<Entities::Cube>(material0);
		//auto cube1 = world.SpawnEntity<Entities::Cube>(2.0f, 0.0f, 0.0f, 1.0f, material1);

		// TEST CODE: Spawn loads of cubes to test performance.
		usize iterations = round(sqrt(NUM_ENTITIES));
		for (usize i = 0; i < iterations; ++i)
		{
			for (usize j = 0; j < iterations; ++j)
			{
				world.SpawnEntity<Entities::Cube>(i * 2.0f, 0.0f, j * 2.0f, 1.0f, ((i + j) % 2 == 0) ? material0 : material1);
			}
		}
	}

	// Run the main loop of the engine.
	world.Loop();

	// Shutdown and cleanup the world.
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
