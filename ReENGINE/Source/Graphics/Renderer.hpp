/*
 * Renderer.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"
#include "Core/Entity.hpp"

#include <boost/container/vector.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

namespace Re
{
	namespace Graphics
	{
		class Renderer
		{
		private:
			struct RenderThreadData
			{
				usize _index;
				bool _shouldClose;
				boost::thread _handle;

				RenderThreadData()
					: _index(-1), _shouldClose(false) {}
			};

		public:
			Renderer();

			void AddToQueue(boost::shared_ptr<Core::Entity> newEntity);
			void Startup();
			void Shutdown();

		private:
			void DrawToQueue(usize threadIndex);
			void JoinRenderThreads();

		private:
			boost::lockfree::queue<Core::Entity*> _drawingQueue;
			boost::container::vector<RenderThreadData> _drawingThreadsData;
			boost::container::vector<boost::mutex> _drawingThreadsMutex;
			boost::condition_variable _drawingAvailable;
			
		};
	}
}
