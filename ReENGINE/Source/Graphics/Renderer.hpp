/*
 * Renderer.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"
#include "Core/Entity.hpp"
#include "Core/Result.hpp"

#include <boost/container/vector.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#if PLATFORM_WINDOWS
#include "Platform/Win32/Window.hpp"
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

namespace Re
{
	namespace Graphics
	{
		enum class RendererResult
		{
			Success = 0,
			Failure = 1
		};

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

			RendererResult Startup(const Platform::Win32Window& window);
			void Shutdown();

		private:
			// Multithreading private methods.

			void DrawToQueue(usize threadIndex);
			void JoinRenderThreads();

			// Vulkan-related private methods.

			RendererResult CreateInstance();

			#if PLATFORM_WINDOWS
			RendererResult CreateWindowsSurface(const Platform::Win32Window& window);
			#endif

			#if _DEBUG
			RendererResult CreateDebugCallback();
			void DestroyDebugCallback();
			#endif

		private:
			// Multithreading members.
			boost::lockfree::queue<Core::Entity*> _drawingQueue;
			boost::container::vector<RenderThreadData> _drawingThreadsData;
			boost::container::vector<boost::mutex> _drawingThreadsMutex;
			boost::condition_variable _drawingAvailable;
			
			// Vulkan-related members.
			VkInstance _instance;
			VkSurfaceKHR _surface;

			#if _DEBUG
			VkDebugUtilsMessengerEXT _debugMessenger;
			#endif

		};
	}
}
