/*
 * Renderer.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Renderer.hpp"

#include <boost/container/set.hpp>

#define NUM_RENDER_THREADS 16

static const boost::container::vector<const utf8*> instanceExtensions = {
	VK_KHR_SURFACE_EXTENSION_NAME,

	#if PLATFORM_WINDOWS
	VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
	#endif

	#if _DEBUG
	VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
	#endif
};

static const boost::container::vector<const utf8*> deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
};

#if _DEBUG

static const boost::container::vector<const utf8*> instanceLayers = {
	"VK_LAYER_KHRONOS_validation",
};

static VKAPI_ATTR VkBool32 VKAPI_CALL vkDebugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
													  VkDebugUtilsMessageTypeFlagsEXT messageType,
													  const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
													  void* pUserData)
{
	printf("VULKAN DEBUG CALLBACK: %s\n", pCallbackData->pMessage);
	return VK_FALSE;
}

#else

static const boost::container::vector<const utf8*> instanceLayers = {};

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
			CHECK_RESULT_WITH_ERROR(RetrievePhysicalDevice(), RendererResult::Success, NTEXT("Failed to retrieve appropriate physical device!\n"), RendererResult::Failure);
			CHECK_RESULT_WITH_ERROR(CreateLogicalDevice(), RendererResult::Success, NTEXT("Failed to create logical device!\n"), RendererResult::Failure);

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
			vkDestroyDevice(_device._logical, nullptr);
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

		RendererResult Renderer::RetrievePhysicalDevice()
		{
			u32 deviceCount = 0;
			CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr), VK_SUCCESS, RendererResult::Failure);

			// If there aren't any devices available, then Vulkan is not supported.
			if (deviceCount == 0)
			{
				return RendererResult::Failure;
			}

			boost::container::vector<VkPhysicalDevice> devices(deviceCount);
			CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data()), VK_SUCCESS, RendererResult::Failure);

			// Pick an appropriate physical device from the available list.
			for (auto& dev : devices)
			{
				if (CheckPhysicalDeviceSuitable(dev))
				{
					_device._physical = dev;
					return RendererResult::Success;
				}
			}

			return RendererResult::Failure;
		}

		bool Renderer::CheckDeviceExtensionSupport(VkPhysicalDevice device) const
		{
			u32 extensionCount = 0;
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

			// If no extensions have been found, don't even bother looking for.
			if (extensionCount == 0)
			{
				return false;
			}

			boost::container::vector<VkExtensionProperties> extensions(extensionCount);
			vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, extensions.data());

			// Check if required extensions are supported by physical device.
			for (const auto& reqExt : deviceExtensions)
			{
				bool hasExtension = false;
				for (const auto& ext : extensions)
				{
					if (strcmp(reqExt, ext.extensionName) == 0)
					{
						hasExtension = true;
						break;
					}
				}

				if (!hasExtension)
				{
					return false;
				}
			}

			return true;
		}

		bool Renderer::CheckPhysicalDeviceSuitable(VkPhysicalDevice device) const
		{
			auto queueFamilies = GetQueueFamilyInfo(device);
			bool extensionsSupported = CheckDeviceExtensionSupport(device);
			auto swapchainInfo = GetSwapchainInfo(device);
			bool swapchainValid = !swapchainInfo._formats.empty() && !swapchainInfo._modes.empty();

			return queueFamilies.IsValid() && extensionsSupported && swapchainValid;
		}

		Renderer::QueueFamilyInfo Renderer::GetQueueFamilyInfo(VkPhysicalDevice device) const
		{
			u32 familyCount = 0;
			vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, nullptr);

			boost::container::vector<VkQueueFamilyProperties> familyProperties(familyCount);
			vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, familyProperties.data());

			Renderer::QueueFamilyInfo familyInfo;
			for (usize i = 0; i < familyCount; ++i)
			{
				if (familyProperties[i].queueCount > 0 && familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)
				{
					familyInfo._graphicsCount = familyProperties[i].queueCount;
					familyInfo._graphicsFamily = i;
				}

				// Check if queue family supports presentation.
				VkBool32 presentationSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentationSupport);
				if (familyProperties[i].queueCount > 0 && presentationSupport)
				{
					familyInfo._presentationFamily = i;
				}

				if (familyInfo.IsValid())
				{
					break;
				}
			}

			return familyInfo;
		}

		Renderer::SwapchainInfo Renderer::GetSwapchainInfo(VkPhysicalDevice device) const
		{
			Renderer::SwapchainInfo info;

			vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &info._capabilities);

			u32 formatCount = 0;
			vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);
			if (formatCount != 0)
			{
				info._formats.resize(formatCount);
				vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, info._formats.data());
			}

			u32 modesCount = 0;
			vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &modesCount, nullptr);
			if (modesCount != 0)
			{
				info._modes.resize(modesCount);
				vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &modesCount, info._modes.data());
			}

			return info;
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
			createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
			createInfo.ppEnabledExtensionNames = instanceExtensions.data();
			createInfo.enabledLayerCount = static_cast<uint32_t>(instanceLayers.size());
			createInfo.ppEnabledLayerNames = instanceLayers.data();

			CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &_instance), VK_SUCCESS, RendererResult::Failure);
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateLogicalDevice()
		{
			auto familyInfo = GetQueueFamilyInfo(_device._physical);
			boost::container::set<i32> queueFamilies = { familyInfo._graphicsFamily, familyInfo._presentationFamily };
			boost::container::vector<VkDeviceQueueCreateInfo> queueInfos(queueFamilies.size());
			boost::container::vector<boost::container::vector<float>> queueProrities(queueFamilies.size());
			boost::container::map<i32, i32> queueCounts;

			// Add queue counts into the map with unique keys.
			queueCounts.emplace_unique(familyInfo._graphicsFamily, familyInfo._graphicsCount);
			queueCounts.emplace_unique(familyInfo._presentationFamily, 1);

			// Create the information regarding the queues that will be used from the logical device.
			for (auto& family : queueFamilies)
			{
				// Configure queue priorities for specified family.
				queueProrities[family].resize(queueCounts[family]);
				queueProrities[family].assign(queueCounts[family], 1.0f);

				// Configure queue info for specified family.
				queueInfos[family].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfos[family].queueFamilyIndex = family;
				queueInfos[family].queueCount = queueCounts[family];
				queueInfos[family].pQueuePriorities = queueProrities[family].data();
			}

			// Construct the features from the device that we are going to use.
			VkPhysicalDeviceFeatures features = {};

			// Create the information regarding the logical device. (ignore layers, they are deprecated).
			VkDeviceCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
			createInfo.queueCreateInfoCount = static_cast<u32>(queueInfos.size());
			createInfo.pQueueCreateInfos = queueInfos.data();
			createInfo.enabledExtensionCount = static_cast<u32>(deviceExtensions.size());
			createInfo.ppEnabledExtensionNames = deviceExtensions.data();
			createInfo.pEnabledFeatures = &features;

			CHECK_RESULT(vkCreateDevice(_device._physical, &createInfo, nullptr, &_device._logical), VK_SUCCESS, RendererResult::Failure);

			// Retrieve the queues created by the logical device and store handles.
			_graphicsQueues.resize(familyInfo._graphicsCount);
			for (usize i = 0; i < familyInfo._graphicsCount; ++i)
			{
				vkGetDeviceQueue(_device._logical, familyInfo._graphicsFamily, i, &_graphicsQueues[i]);
			}

			vkGetDeviceQueue(_device._logical, familyInfo._presentationFamily, 0, &_presentationQueue);
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
			createInfo.pfnUserCallback = vkDebugCallback;
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
