/*
 * Renderer.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Renderer.hpp"

#define NUM_RENDER_THREADS 16

static const boost::container::vector<const utf8*> vulkanExtensions = {
	VK_KHR_SURFACE_EXTENSION_NAME,

	#if PLATFORM_WINDOWS
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	#endif

	#if _DEBUG
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	#endif
};

#if _DEBUG

static const boost::container::vector<const utf8*> vulkanLayers = {
	"VK_LAYER_KHRONOS_validation",
};

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													VkDebugUtilsMessageTypeFlagsEXT messageType,
													const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
													void* pUserData)
{
	printf("VULKAN DEBUG CALLBACK: %s\n", pCallbackData->pMessage);
	return VK_FALSE;
}

#else

static const boost::container::vector<const utf8*> vulkanLayers = {};

#endif

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

		RendererResult Renderer::Startup(const Platform::Win32Window& window)
		{
			// Vulkan startup
			CHECK_RESULT_WITH_ERROR(CreateInstance(), RendererResult::Success, NTEXT("Failed to create instance!\n"), RendererResult::Failure);
			#if _DEBUG
			CHECK_RESULT_WITH_ERROR(CreateDebugCallback(), RendererResult::Success, NTEXT("Failed to create debug callback!\n"), RendererResult::Failure);
			#endif
			#if PLATFORM_WINDOWS
			CHECK_RESULT_WITH_ERROR(CreateWindowsSurface(window), RendererResult::Success, NTEXT("Failed to create surface in Windows!\n"), RendererResult::Failure);
			#endif

			// Multithreading startup
			_drawingThreadsData.resize(NUM_RENDER_THREADS);
			for (usize i = 0; i < NUM_RENDER_THREADS; ++i)
			{
				RenderThreadData threadData = {};
				threadData._index = i;
				threadData._shouldClose = false;
				threadData._handle = boost::thread(boost::bind(&Renderer::DrawToQueue, this, i));
				
				_drawingThreadsData[i] = std::move(threadData);
			}

			return RendererResult::Success;
		}

		void Renderer::Shutdown()
		{
			// Multithreading shutdown
			JoinRenderThreads();
			_drawingThreadsData.clear();

			// Vulkan shutdown
			vkDestroySurfaceKHR(_instance, _surface, nullptr);
			#if _DEBUG
			DestroyDebugCallback();
			#endif
			vkDestroyInstance(_instance, nullptr);
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
					//printf("Drawing Entity %d\n", entity->GetId());
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

		RendererResult Renderer::CreateInstance()
		{
			VkApplicationInfo applicationInfo = {};
			applicationInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
			applicationInfo.engineVersion = VK_MAKE_VERSION(0, 1, 0);
			applicationInfo.pEngineName = "ReENGINE";
			applicationInfo.applicationVersion = VK_MAKE_VERSION(0, 0, 0);
			applicationInfo.pApplicationName = "Test Application";
			applicationInfo.apiVersion = VK_API_VERSION_1_2;

			VkInstanceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
			createInfo.pApplicationInfo = &applicationInfo;
			createInfo.enabledExtensionCount = static_cast<uint32_t>(vulkanExtensions.size());
			createInfo.ppEnabledExtensionNames = vulkanExtensions.data();
			createInfo.enabledLayerCount = static_cast<uint32_t>(vulkanLayers.size());
			createInfo.ppEnabledLayerNames = vulkanLayers.data();

			CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &_instance), VK_SUCCESS, RendererResult::Failure);
			return RendererResult::Success;
		}

		#if PLATFORM_WINDOWS
		RendererResult Renderer::CreateWindowsSurface(const Platform::Win32Window& window)
		{
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hinstance = window.GetInstance();
			createInfo.hwnd = window.GetHandle();

			CHECK_RESULT(vkCreateWin32SurfaceKHR(_instance, &createInfo, nullptr, &_surface), VK_SUCCESS, RendererResult::Failure);
			return RendererResult::Success;
		}

		#endif

		#if _DEBUG
		RendererResult Renderer::CreateDebugCallback()
		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
				VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = DebugCallback;
			createInfo.pUserData = nullptr;

			auto createFunction = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
			CHECK_RESULT(createFunction(_instance, &createInfo, nullptr, &_debugMessenger), VK_SUCCESS, RendererResult::Failure);
			return RendererResult::Success;
		}

		void Renderer::DestroyDebugCallback()
		{
			auto destroyFunction = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
			destroyFunction(_instance, _debugMessenger, nullptr);
		}
		#endif
	}
}
