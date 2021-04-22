/*
 * World.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "World.hpp"

#include "Components/RenderComponent.hpp"
#include "Entities/Cube.hpp"

namespace Re
{
	namespace Core
	{
		World::World()
			: _dispatchQueue(128), _dispatchThreadShouldClose(false)
		{}

		void World::AddEntity(const boost::shared_ptr<Core::Entity>& entity)
		{
			_dispatchQueue.push(entity.get());
			_shouldDispatch.notify_one();
		}

		void World::AddEntity(const boost::shared_ptr<Entities::Camera>& camera)
		{
			_renderer.SetActiveCamera(camera);
		}

		void World::AddEntity(const boost::shared_ptr<Entities::DirectionalLight>& light)
		{
			_renderer.ActivateLight(light);
		}

		void World::AddEntity(const boost::shared_ptr<Entities::PointLight>& light)
		{
			_renderer.ActivateLight(light);
		}

		void World::AddEntity(const boost::shared_ptr<Entities::SpotLight>& light)
		{
			_renderer.ActivateLight(light);
		}

		void World::DispatchToRenderer()
		{
			while (true)
			{
				boost::unique_lock<boost::mutex> lock(_dispatchThreadMutex);
				_shouldDispatch.wait(lock, [this] { return _dispatchThreadShouldClose || !_dispatchQueue.empty(); });

				if (_dispatchThreadShouldClose)
				{
					return;
				}
				else
				{
					// Dispatch new entities to the renderer.
					Entity* newEntity;
					while (_dispatchQueue.pop(newEntity))
					{
						_renderer.AddEntity(newEntity);
					}
				}
			}
		}

		void World::JoinDispatchThreads()
		{
			// Set up thread so that it may close.
			_dispatchThreadShouldClose = true;

			// Notify all threads so that they may wake up and close.
			_shouldDispatch.notify_all();

			// Join dispatch thread.
			_dispatchThread.join();
		}

		WorldResult World::Startup()
		{
			CHECK_RESULT(_window.Startup("Test Application", 1024, 768, SW_SHOW), Platform::WindowResult::Success, WorldResult::Failure);
			CHECK_RESULT(_renderer.Startup(_window), Graphics::RendererResult::Success, WorldResult::Failure);

			// Initialize dispatching thread.
			_dispatchThreadShouldClose = false;
			_dispatchThread = boost::thread(boost::bind(&World::DispatchToRenderer, this));

			return WorldResult::Success;
		}

		void World::Shutdown()
		{
			JoinDispatchThreads();
			_entities.clear();
			_renderer.Shutdown();
			_window.Shutdown();
		}

		void World::Loop()
		{
			bool bAdded = false;
			u32 frames = 0;
			f32 elapsed = 0;

			_timer.Reset();
			_timer.Start();
			while (!_window.GetShouldClose())
			{
				// Game Loop design pattern.
				_timer.Tick();
				_window.PollEvents();
				Update();
				_renderer.Render();
				
				// Calculate FPS and print to console output.
				if (elapsed >= 1.0f)
				{
					printf("FPS: %.2f\n", (f32)frames / elapsed);
					elapsed = 0.0f;
					frames = 0;

					// TEST CODE: Spawn a cube every second.
					// SpawnEntity<Entities::Cube>(rand() % 50, 0.0f, rand() % 50, 1.0f);
				}

				frames++;
				elapsed += _timer.DeltaTime();
			}
		}

		void World::Update()
		{
			for (auto& entity : _entities)
			{
				entity.second->Update(_timer.DeltaTime());
			}
		}
	}
}
