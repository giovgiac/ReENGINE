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
			// Multithreaded-related structures.

			struct RenderThreadData
			{
				usize _index;
				bool _shouldClose;
				boost::thread _handle;

				RenderThreadData()
					: _index(-1), _shouldClose(false) {}
			};

			// Vulkan-related structures.

			struct QueueFamilyInfo
			{
				i32 _graphicsCount = 0;
				i32 _graphicsFamily = -1;
				i32 _presentationFamily = -1;

				bool IsValid() const
				{
					return _graphicsFamily >= 0 && _presentationFamily >= 0;
				}
			};

			struct SwapchainInfo 
			{
				VkSurfaceCapabilitiesKHR _capabilities;
				boost::container::vector<VkSurfaceFormatKHR> _formats;
				boost::container::vector<VkPresentModeKHR> _modes;
			};

			struct SwapchainImage
			{
				VkImage _raw;
				VkImageView _view;
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

			// Get functions.
			RendererResult RetrievePhysicalDevice();

			// Support functions.
			bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
			bool CheckPhysicalDeviceSuitable(VkPhysicalDevice device) const;
			QueueFamilyInfo GetQueueFamilyInfo(VkPhysicalDevice device) const;
			SwapchainInfo GetSwapchainInfo(VkPhysicalDevice device) const;

			// Support create functions.
			RendererResult CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags flags, VkImageView* outView) const;
			RendererResult CreateShaderModule(const boost::container::vector<char>& raw, VkShaderModule* outModule) const;

			// Choose functions.
			VkSurfaceFormatKHR ChooseBestSurfaceFormat(const boost::container::vector<VkSurfaceFormatKHR>& formats) const;
			VkPresentModeKHR ChooseBestPresentationMode(const boost::container::vector<VkPresentModeKHR>& modes) const;
			VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

			// Create functions.
			RendererResult CreateInstance();
			RendererResult CreateLogicalDevice();
			RendererResult CreateSwapchain();
			RendererResult CreateRenderPass();
			RendererResult CreateGraphicsPipeline();

			// Destroy functions.
			void DestroySwapchain();

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
			struct {
				VkPhysicalDevice _physical;
				VkDevice _logical;
			} _device;
			boost::container::vector<VkQueue> _graphicsQueues;
			VkQueue _presentationQueue;
			VkSwapchainKHR _swapchain;
			boost::container::vector<SwapchainImage> _swapchainImages;

			// Pipeline-related members.
			VkPipeline _graphicsPipeline;
			VkPipelineLayout _pipelineLayout;
			VkRenderPass _renderPass;

			#if _DEBUG
			VkDebugUtilsMessengerEXT _debugMessenger;
			#endif

			// Vulkan configuration members.
			VkFormat _swapchainFormat;
			VkExtent2D _swapchainExtent;

			const Platform::Win32Window* _window;

		};
	}
}
