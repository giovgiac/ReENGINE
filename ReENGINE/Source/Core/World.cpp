/*
 * World.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "World.hpp"

namespace Re
{
	namespace Core
	{
		World::World()
			: _dispatchQueue(128), _dispatchThreadShouldClose(false) //_dispatchThreadsMutex(NUM_DISPATCH_THREADS)
		{}

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
			_timer.Reset();
			_timer.Start();
			while (!_window.GetShouldClose())
			{
				_window.PollEvents();
				_timer.Tick();
				_renderer.Render();
			}
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
	}
}
