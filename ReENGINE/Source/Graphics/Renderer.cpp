/*
 * Renderer.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Renderer.hpp"

#define NUM_RENDER_THREADS 16

namespace Re
{
	namespace Graphics
	{
		Renderer::Renderer()
			: _drawingQueue(512), _drawingThreadsMutex(NUM_RENDER_THREADS)
		{}

		void Renderer::AddToQueue(boost::shared_ptr<Core::Entity> newEntity)
		{
			_drawingQueue.push(newEntity.get());
			_drawingAvailable.notify_one();
		}

		void Renderer::Startup()
		{
			_drawingThreadsData.resize(NUM_RENDER_THREADS);
			for (usize i = 0; i < NUM_RENDER_THREADS; ++i)
			{
				RenderThreadData threadData = {};
				threadData._index = i;
				threadData._shouldClose = false;
				threadData._handle = boost::thread(boost::bind(&Renderer::DrawToQueue, this, i));
				
				_drawingThreadsData[i] = std::move(threadData);
			}
		}

		void Renderer::Shutdown()
		{
			JoinRenderThreads();
			_drawingThreadsData.clear();
		}

		void Renderer::DrawToQueue(usize threadIndex)
		{
			RenderThreadData& threadData = _drawingThreadsData[threadIndex];
			
			while (true)
			{
				Core::Entity* entity;

				// Wait for entities to be available in drawing queue.
				boost::unique_lock<boost::mutex> lock(_drawingThreadsMutex[threadIndex]);
				_drawingAvailable.wait(lock, [this, &threadData] { return threadData._shouldClose || !_drawingQueue.empty(); });

				if (threadData._shouldClose)
				{
					return;
				}
				else if (_drawingQueue.pop(entity))
				{
					// TODO: Generate commands to draw entity.
					//Core::Debug::Log(NTEXT("Drawing Entity %d\n"), entity->GetId());
					printf("Drawing Entity %d\n", entity->GetId());
				}
			}
		}

		void Renderer::JoinRenderThreads()
		{
			// Set threads up so that they may close.
			std::for_each(_drawingThreadsData.begin(), _drawingThreadsData.end(), [](RenderThreadData& threadData) { 
				threadData._shouldClose = true; 
			});

			// Notify all threads so that they may wake up and close.
			_drawingAvailable.notify_all();

			// Join all the threads one by one.
			std::for_each(_drawingThreadsData.begin(), _drawingThreadsData.end(), [](RenderThreadData& threadData) { 
				threadData._handle.join();
			});
		}
	}
}
