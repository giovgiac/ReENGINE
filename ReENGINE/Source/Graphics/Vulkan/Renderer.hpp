/*
 * Renderer.hpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#pragma once

#include "Core/Debug/Assert.hpp"

#ifdef PLATFORM_USE_VULKAN

#include "Core/Entity.hpp"
#include "Core/Result.hpp"
#include "Graphics/Vertex.hpp"
#include "Math/Vector3.hpp"

#include <boost/atomic.hpp>
#include <boost/container/map.hpp>
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
			// Vulkan-related structures.

			struct QueueFamilyInfo
			{
				i32 _graphicsFamily = -1;
				i32 _presentationFamily = -1;

				bool IsValid() const
				{
					return _graphicsFamily >= 0 && _presentationFamily >= 0;
				}
			};

			struct RenderInfo
			{
				VkBuffer _vertexBuffer;
				VkDeviceMemory _vertexMemory;
				i32 _vertexCount;
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

			bool AddEntity(Core::Entity* newEntity);
			bool RemoveEntity(Core::Entity* entityToRemove);
			
			RendererResult Startup(const Platform::Win32Window& window);
			RendererResult Render();
			void Shutdown();

		private:
			// Multithreading private methods.
			//void DrawToQueue();
			//void JoinRenderThreads();

			// Vulkan-related private methods.

			// Get functions.
			RendererResult RetrievePhysicalDevice();

			// Support functions.
			bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
			bool CheckPhysicalDeviceSuitable(VkPhysicalDevice device) const;
			u32 FindMemoryTypeIndex(u32 allowedTypes, VkMemoryPropertyFlags flags) const;
			QueueFamilyInfo GetQueueFamilyInfo(VkPhysicalDevice device) const;
			SwapchainInfo GetSwapchainInfo(VkPhysicalDevice device) const;

			// Support create functions.
			RendererResult CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags flags, VkImageView* outView) const;
			RendererResult CreateShaderModule(const boost::container::vector<char>& raw, VkShaderModule* outModule) const;
			RendererResult CreateVertexBuffer(boost::container::vector<Vertex>& vertices, VkBuffer* outBuffer, VkDeviceMemory* outMemory) const;

			// Support destroy functions.
			void DestroyVertexBuffer(VkBuffer buffer, VkDeviceMemory memory) const;

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
			RendererResult CreateFramebuffers();
			RendererResult CreateCommandPools();
			RendererResult CreateCommandBuffers();
			RendererResult CreateSynchronization();

			// Record Functions.
			RendererResult RecordCommands();

			// Destroy functions.
			void DestroyEntities();
			void DestroySwapchain();
			void DestroyFramebuffers();
			void DestroySynchronization();

			#if PLATFORM_WINDOWS
			RendererResult CreateWindowsSurface(const Platform::Win32Window& window);
			#endif

			#if _DEBUG
			RendererResult CreateDebugCallback();
			void DestroyDebugCallback();
			#endif

		private:
			// Multithreading members.
			//boost::lockfree::queue<Core::Entity*> _additionQueue;
			//boost::thread _additionThread;
			//boost::mutex _additionMutex;
			//boost::condition_variable _entityAvailable;
			//bool _additionThreadShouldClose;

			//boost::container::vector<RenderThreadData> _drawingThreadsData;
			//boost::container::vector<boost::mutex> _drawingThreadsMutex;
			
			// Vulkan-related members.
			VkInstance _instance;
			VkSurfaceKHR _surface;
			struct {
				VkPhysicalDevice _physical;
				VkDevice _logical;
			} _device;
			VkQueue _graphicsQueue;
			VkQueue _presentationQueue;
			VkSwapchainKHR _swapchain;
			boost::container::vector<SwapchainImage> _swapchainImages;
			boost::container::vector<VkFramebuffer> _swapchainFramebuffers;
			boost::container::vector<VkCommandBuffer> _commandBuffers;
			i32 _currentFrame;

			// Pipeline-related members.
			VkPipeline _graphicsPipeline;
			VkPipelineLayout _pipelineLayout;
			VkRenderPass _renderPass;
			VkCommandPool _graphicsPool;

			// Vulkan configuration members.
			VkFormat _swapchainFormat;
			VkExtent2D _swapchainExtent;

			// Vulkan synchronization members.
			boost::container::vector<VkSemaphore> _imageAvailable;
			boost::container::vector<VkSemaphore> _renderFinished;
			boost::container::vector<VkFence> _drawFences;
			boost::atomic<bool> _recordingCommands = true;

			// Entity-related members.
			boost::container::map<Core::Entity*, RenderInfo> _entitiesToRender;

			#if _DEBUG
			VkDebugUtilsMessengerEXT _debugMessenger;
			#endif

			const Platform::Win32Window* _window;

		};
	}
}

#endif // PLATFORM_USE_VULKAN
