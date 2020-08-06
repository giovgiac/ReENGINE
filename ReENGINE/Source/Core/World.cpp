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
			CHECK_RESULT(_window.Startup("Test Application", 800, 600, SW_SHOW), Platform::WindowResult::Success, WorldResult::Failure);
			CHECK_RESULT(_renderer.Startup(_window), Graphics::RendererResult::Success, WorldResult::Failure);

			// Initialize dispatching thread.
			_dispatchThreadShouldClose = false;
			_dispatchThread = boost::thread(boost::bind(&World::DispatchToRenderer, this));
			/*
			_dispatchThreadsData.resize(NUM_DISPATCH_THREADS);
			for (usize i = 0; i < NUM_DISPATCH_THREADS; ++i)
			{
				DispatchThreadData threadData = {};
				threadData._index = i;
				threadData._shouldClose = false;
				threadData._handle = boost::thread(boost::bind(&World::DispatchToRenderer, this, i));

				_dispatchThreadsData[i] = std::move(threadData);
			}
			*/

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
				Entity* newEntity;
				boost::unique_lock<boost::mutex> lock(_dispatchThreadMutex);
				_shouldDispatch.wait(lock, [this] { return _dispatchThreadShouldClose || !_dispatchQueue.empty(); });

				if (_dispatchThreadShouldClose)
				{
					return;
				}
				else
				{
					// Dispatch new entities to the renderer.
					while (_dispatchQueue.pop(newEntity))
					{
						_renderer.AddEntity(newEntity);
					}
				}
			}

			/*
			DispatchThreadData& threadData = _dispatchThreadsData[threadIndex];
			while (true)
			{
				boost::unique_lock<boost::mutex> lock(_dispatchThreadsMutex[threadIndex]);
				_shouldDispatch.wait(lock, [&threadData] { return threadData._shouldClose || threadData._shouldRender; });

				if (threadData._shouldClose)
				{
					return;
				}
				else if (threadData._shouldRender)
				{
					// TODO: Fix algorithm, only works on multiples of NUM_DISPATCH_THREADS.
					auto current = _entities.cbegin();
					auto end = _entities.cbegin();
					usize blockSize = (_entities.size() + 1) / NUM_DISPATCH_THREADS;
					
					// Performing dispatching to renderer.
					std::advance(current, threadIndex * blockSize);
					std::advance(end, (threadIndex + 1) * blockSize);
					for (; current != end; ++current)
					{
						if (current.get())
						{
							_renderer.AddToQueue(current->second);
						}
					}

					threadData._shouldRender = false;
				}
			}
			*/
		}

		void World::JoinDispatchThreads()
		{
			// Set up thread so that it may close.
			_dispatchThreadShouldClose = true;
			//std::for_each(_dispatchThreadsData.begin(), _dispatchThreadsData.end(), [](DispatchThreadData& threadData) { 
			//	threadData._shouldClose = true; 
			//});

			// Notify all threads so that they may wake up and close.
			_shouldDispatch.notify_all();

			// Join dispatch thread.
			_dispatchThread.join();

			//std::for_each(_dispatchThreadsData.begin(), _dispatchThreadsData.end(), [](DispatchThreadData& threadData) {
			//	threadData._handle.join(); 
			//});
		}
	}
}
