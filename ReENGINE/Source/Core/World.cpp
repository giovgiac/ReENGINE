/*
 * World.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "World.hpp"

#define NUM_DISPATCH_THREADS 8

namespace Re
{
	namespace Core
	{
		World::World()
			: _dispatchThreadsMutex(NUM_DISPATCH_THREADS)
		{}

		WorldResult World::Startup()
		{
			CHECK_RESULT(_window.Startup("Test Application", 800, 600, SW_SHOW), Platform::WindowResult::Success, WorldResult::Failure);
			CHECK_RESULT(_renderer.Startup(_window), Graphics::RendererResult::Success, WorldResult::Failure);

			_dispatchThreadsData.resize(NUM_DISPATCH_THREADS);
			for (usize i = 0; i < NUM_DISPATCH_THREADS; ++i)
			{
				DispatchThreadData threadData = {};
				threadData._index = i;
				threadData._shouldClose = false;
				threadData._shouldRender = false;
				threadData._handle = boost::thread(boost::bind(&World::DispatchToRenderer, this, i));

				_dispatchThreadsData[i] = std::move(threadData);
			}

			return WorldResult::Success;
		}

		void World::Shutdown()
		{
			JoinDispatchThreads();
			_dispatchThreadsData.clear();
			_entities.clear();
			_renderer.Shutdown();
			_window.Shutdown();
		}

		void World::Render()
		{
			for (auto& threadData : _dispatchThreadsData)
			{
				if (threadData._shouldRender == false)
				{
					threadData._shouldRender = true;
				}
			}

			_shouldDispatch.notify_all();
		}

		void World::Loop()
		{
			_timer.Reset();
			_timer.Start();
			while (!_window.GetShouldClose())
			{
				_window.PollEvents();
				Render();
				_timer.Tick();
			}
		}

		void World::DispatchToRenderer(usize threadIndex)
		{
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
		}

		void World::JoinDispatchThreads()
		{
			// Set up threads so that they may close.
			std::for_each(_dispatchThreadsData.begin(), _dispatchThreadsData.end(), [](DispatchThreadData& threadData) { 
				threadData._shouldClose = true; 
			});

			// Notify all threads so that they may wake up and close.
			_shouldDispatch.notify_all();

			// Join all threads one by one.
			std::for_each(_dispatchThreadsData.begin(), _dispatchThreadsData.end(), [](DispatchThreadData& threadData) {
				threadData._handle.join(); 
			});
		}
	}
}
