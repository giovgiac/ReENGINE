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
#include "Entities/Model.hpp"
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
	
	void Draw(const Platform::ITimer&) override {}
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

	void Update(const Platform::ITimer&) override {}
};

// Newton Manager
NewtonManager nNewtonManager;
TestGame nGameManager;

const usize NUM_ENTITIES = 16;

// TESTING CLASS - TO BE FULLY IMPLEMENTED LATER
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
		auto camera = world.SpawnEntity<Entities::Camera>(60.0f, 0.1f, 1000.0f);
		camera->GetTransform()->Translate(0.0f, 2.0f, 16.0f);

		// TEST CODE: Create lights to illuminate the world.
		// auto directionalLight = world.SpawnEntity<Entities::DirectionalLight>(Math::Colors::LightYellow, Math::Vector3(2.0f, 1.0f, -2.0f), 0.05f, 1.0f);
		// auto pointLight0 = world.SpawnEntity<Entities::PointLight>(Math::Colors::RoyalBlue, Math::Vector3(0.0f, 1.0f, 0.0f), 0.0f, 1.0f, 0.2f, 0.1f);
		auto spotLight0 = world.SpawnEntity<Entities::SpotLight>(Math::Colors::GhostWhite, Math::Vector3(0.0f, 2.0f, 16.0f), Math::Vector3(0.0f, 0.0f, -1.0f), 30.0f, 0.25f, 1.0f, 0.1f, 0.05f);
		camera->GetTransform()->OnTransformChanged.connect([camera, spotLight0]() {
			spotLight0->SetPosition(camera->GetTransform()->GetPosition());
			spotLight0->SetDirection(camera->GetTransform()->GetTransform().Forward());
		});

		// TEST CODE: Create some test textures.
		auto texture0 = boost::make_shared<Graphics::Texture>("Textures/brick.png");
		auto texture1 = boost::make_shared<Graphics::Texture>("Textures/dirt.png");

		// TEST CODE: Create some test materials.
		auto material0 = boost::make_shared<Graphics::Material>(32.0f, 1.0f, texture0);
		auto material1 = boost::make_shared<Graphics::Material>(2.0f, 0.25f, texture1);
		auto material2 = boost::make_shared<Graphics::Material>(1.0f, 0.0f);

		// TEST CODE: Assemble materials into boost array.
		boost::array<boost::shared_ptr<Graphics::Material>, 3> materials = {
			material0, material1, material2
		};

		// TEST CODE: Spawn loads of cubes to test performance.
		usize iterations = round(sqrt(NUM_ENTITIES));
		for (usize i = 0; i < iterations; ++i)
		{
			for (usize j = 0; j < iterations; ++j)
			{
				world.SpawnEntity<Entities::Cube>(i * 2.0f - (iterations - 1), 0.0f, j * 2.0f - (iterations - 1), 1.0f, materials[rand() % materials.size()]);
			}
		}

		// TEST CODE: Load an external 3D model and textures.
		auto model0 = world.SpawnEntity<Entities::Model>("Models/uh60.obj");
		model0->GetTransform()->Rotate(-90.0f, 0.0f, 0.0f);
		model0->GetTransform()->Translate(0.0f, 4.0f, 0.0f);

		auto model1 = world.SpawnEntity<Entities::Model>("Models/x-wing.obj");
		model1->GetTransform()->Translate(-14.0f, 0.5f, 6.0f);
		model1->GetTransform()->Scale(0.01f);

		//auto scene = world.SpawnEntity<Entities::Model>("Models/scene.obj");

		//auto terrain = world.SpawnEntity<Entities::Model>("Models/terrain.obj");
		//terrain->GetTransform()->Scale(0.5);
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
