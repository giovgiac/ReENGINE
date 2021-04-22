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
#include "Entities/Camera.hpp"
#include "Entities/DirectionalLight.hpp"
#include "Entities/PointLight.hpp"
#include "Entities/SpotLight.hpp"
#include "Graphics/Material.hpp"
#include "Graphics/Vertex.hpp"
#include "Math/Matrix.hpp"
#include "Math/Transform.hpp"
#include "Math/Vector3.hpp"

#include <boost/atomic.hpp>
#include <boost/bimap.hpp>
#include <boost/container/map.hpp>
#include <boost/container/set.hpp>
#include <boost/container/vector.hpp>
#include <boost/lockfree/queue.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>

#if PLATFORM_WINDOWS
#include "Platform/Win32/Window.hpp"
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

const usize COMMAND_BUFFER_SETS = 2;
const usize MAX_FRAME_DRAWS = 3;
const usize MAX_ENTITIES = 16384;
const usize MAX_POINT_LIGHTS = 4;
const usize MAX_SPOT_LIGHTS = 4;

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
			// Descriptor-related structures.

			struct VertexUniform 
			{
				alignas(16)	Math::Matrix _projection;
			} _vertexUniform;

			struct FragmentUniform 
			{
				struct FragmentLight 
				{
					alignas(16)	Math::Vector3 _color;
					alignas(4)	f32 _ambientStrength;
					alignas(4)	f32 _diffuseStrength;
				};

				struct FragmentDirectionalLight 
				{
					alignas(16)	FragmentLight _base;
					alignas(16) Math::Vector3 _direction;
				};

				struct FragmentPointLight 
				{
					alignas(16)	FragmentLight _base;
					alignas(16)	Math::Vector3 _position;
					alignas(4)	f32 _constantAttenuation;
					alignas(4)	f32 _linearAttenuation;
					alignas(4)	f32 _quadraticAttenuation;
				};

				struct FragmentSpotLight
				{
					alignas(16)	FragmentPointLight _base;
					alignas(16)	Math::Vector3 _direction;
					alignas(4)	f32 _cutoffAngle;
				};

				alignas(16)	FragmentDirectionalLight _directionalLight;
				alignas(16) FragmentPointLight _pointLights[MAX_POINT_LIGHTS];
				alignas(16) FragmentSpotLight _spotLights[MAX_SPOT_LIGHTS];
				alignas(4)	u32 _pointLightCount;
				alignas(4)	u32 _spotLightCount;
			} _fragmentUniform;

			struct FragmentDynamicUniform 
			{
				struct FragmentMaterial 
				{
					alignas(4)	f32 _specularPower;
					alignas(4)	f32 _specularStrength;
				};

				FragmentMaterial _material;
			};

			struct VertexPush 
			{
				alignas(16)	Math::Matrix _view;
				alignas(16)	Math::Matrix _model;
			};

			// Transfer-related structures.

            struct RenderInfo
            {
				// Vertex-related information.
                VkBuffer _vertexBuffer;
                i32 _vertexCount;
				VkDeviceSize _vertexSize;

				// Index-related information.
                VkBuffer _indexBuffer;
                i32 _indexCount;
				VkDeviceSize _indexSize;

				// Descriptor-related information.
				Components::TransformComponent* _transformComponent;
				Material* _material;
            };

			struct TransferInfo
			{
				Core::Entity* _entity;
				bool _isRemoval;
			};

			struct IndexInfo
			{
				VkBuffer _buffer;
				VkDeviceSize _size;
				boost::container::vector<u32> _indices;
			};

			struct VertexInfo
			{
				VkBuffer _buffer;
				VkDeviceSize _size;
				boost::container::vector<Vertex> _vertices;
			};

			// Vulkan-related structures.

			struct QueueFamilyInfo
			{
				i32 _graphicsFamily = -1;
				i32 _presentationFamily = -1;
				i32 _transferFamily = -1;

				bool HasDedicatedPresentation() const
				{
					return _graphicsFamily != _presentationFamily;
				}

				bool HasDedicatedTransfer() const
				{
					return _graphicsFamily != _transferFamily;
				}

				bool IsValid() const
				{
					return _graphicsFamily >= 0 && _presentationFamily >= 0 && _transferFamily >= 0;
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

			bool AddEntity(Core::Entity* entity);
			bool RemoveEntity(Core::Entity* entityToRemove);
			
			RendererResult Startup(const Platform::Win32Window& window);
			RendererResult Render();
			void Shutdown();

			RendererResult ActivateLight(const boost::shared_ptr<Entities::DirectionalLight>& light);
			RendererResult ActivateLight(const boost::shared_ptr<Entities::PointLight>& light);
			RendererResult ActivateLight(const boost::shared_ptr<Entities::SpotLight>& light);
			void DeactivateLight(const boost::shared_ptr<Entities::PointLight>& light);
			void DeactivateLight(const boost::shared_ptr<Entities::SpotLight>& light);

			void SetActiveCamera(const boost::shared_ptr<Entities::Camera>& newCamera);

		private:
			// Transfer thread private methods.
			void EntityStreaming();

			// Vulkan-related private methods.

			// Get functions.
			RendererResult RetrievePhysicalDevice();

			// Support functions.
			bool CheckDeviceExtensionSupport(VkPhysicalDevice device) const;
			bool CheckPhysicalDeviceSuitable(VkPhysicalDevice device) const;
			u32 FindMemoryTypeIndex(u32 allowedTypes, VkMemoryPropertyFlags flags) const;
			QueueFamilyInfo GetQueueFamilyInfo(VkPhysicalDevice device) const;
			SwapchainInfo GetSwapchainInfo(VkPhysicalDevice device) const;
			usize GetUniformAlignment(usize dataSize) const;

			// Support create functions.
			RendererResult AllocateBuffer(VkBuffer buffer, VkMemoryPropertyFlags properties, VkDeviceMemory* outMemory);
			RendererResult AllocateImage(VkImage image, VkMemoryPropertyFlags properties, VkDeviceMemory* outMemory);
			RendererResult CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* outBuffer);
			RendererResult CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage* outImage);
			RendererResult CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags flags, VkImageView* outView) const;
			RendererResult CreateShaderModule(const boost::container::vector<char>& raw, VkShaderModule* outModule) const;
			RendererResult CreateIndexBuffer(boost::container::vector<u32>& indices, VkBuffer* outBuffer);
			RendererResult CreateVertexBuffer(boost::container::vector<Vertex>& vertices, VkBuffer* outBuffer);

			// Support destroy functions.
			void DestroyBuffer(VkBuffer buffer);

			// Support transfer functions.
			void CleanStageBuffers();
			RendererResult ExecuteTransferOperations();
			RendererResult StageIndexBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory);
			RendererResult StageVertexBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory);

			// Choose functions.
			VkSurfaceFormatKHR ChooseBestSurfaceFormat(const boost::container::vector<VkSurfaceFormatKHR>& formats) const;
			VkPresentModeKHR ChooseBestPresentationMode(const boost::container::vector<VkPresentModeKHR>& modes) const;
			VkFormat ChooseBestSupportedFormat(const boost::container::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) const;
			VkExtent2D ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const;

			// Create functions.
			RendererResult CreateInstance();
			RendererResult CreateLogicalDevice();
			RendererResult CreateSwapchain();
			RendererResult CreateRenderPass();
			RendererResult CreateDescriptorSetLayout();
			RendererResult CreatePushConstantRanges();
			RendererResult CreateGraphicsPipeline();
			RendererResult CreateDepthBufferImage();
			RendererResult CreateFramebuffers();
			RendererResult CreateCommandPools();
			RendererResult CreateCommandBuffers();
			RendererResult CreateUniformBuffers();
			RendererResult CreateDescriptorPool();
			RendererResult CreateDescriptorSets();
			RendererResult CreateSynchronization();

			// Record functions.
			RendererResult RecordCommands(usize buffer, usize offset, usize size, bool isTransfer = false);
			void RecordDraw(usize bufferSet, usize bufferIndex, usize entityIndex, const RenderInfo& renderInfo);

			// Update functions.
			void UpdatePointLight(FragmentUniform::FragmentPointLight* dstLight, const boost::shared_ptr<Entities::PointLight>& srcLight);
			void UpdateSpotLight(FragmentUniform::FragmentSpotLight* dstLight, const boost::shared_ptr<Entities::SpotLight>& srcLight);
			void UpdateVertexUniformBuffers(const usize bufferSet);
			void UpdateFragmentUniformBuffers(const usize bufferSet);
			void UpdateFragmentDynamicUniformBuffers(const usize bufferSet);

			// Destroy functions.
			void DestroyCommandPools();
			void DestroyEntities();
			void DestroySwapchain();
			void DestroyDepthBufferImage();
			void DestroyFramebuffers();
			void DestroySynchronization();
			void DestroyUniformBuffers();

			#if PLATFORM_WINDOWS
			RendererResult CreateWindowsSurface(const Platform::Win32Window& window);
			#endif

			#if _DEBUG
			RendererResult CreateDebugCallback();
			void DestroyDebugCallback();
			#endif

		private:
			// Streaming-related members.
			boost::lockfree::queue<TransferInfo> _streamingQueue;
			boost::thread _streamingThread;
			boost::mutex _streamingMutex;
			boost::condition_variable _streamingRequested;
			boost::atomic<bool> _streamingThreadShouldClose;
			
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
			boost::container::vector<VkCommandBuffer> _commandBuffers[COMMAND_BUFFER_SETS];
			boost::atomic<usize> _currentBuffer;
			usize _currentFrame;

			// Depth-related members.
			VkImage _depthBufferImage;
			VkImageView _depthBufferImageView;
			VkDeviceMemory _depthBufferMemory;

			// Pipeline-related members.
			VkPipeline _graphicsPipeline;
			VkPipelineLayout _pipelineLayout;
			VkRenderPass _renderPass;
			VkCommandPool _graphicsPools[COMMAND_BUFFER_SETS];

			// Transfer-related members.
			VkQueue _transferQueue;
			VkCommandPool _transferPool;
			VkCommandBuffer _transferBuffer;
			boost::bimap<VkBuffer, VkDeviceMemory> _bufferMemory;
			boost::container::vector<VertexInfo> _vertexBuffersToTransfer;
			boost::container::vector<IndexInfo> _indexBuffersToTransfer;
			boost::container::map<Core::Entity*, RenderInfo> _entitiesToTransfer;

			// Descriptor-related members.
			VkDescriptorSetLayout _descriptorSetLayout;
			VkDescriptorPool _descriptorPool;
			boost::container::vector<VkPushConstantRange> _pushConstantRanges;
			boost::container::vector<VkDescriptorSet> _descriptorSets[COMMAND_BUFFER_SETS];

			// Uniform buffer members.
			boost::container::vector<VkBuffer> _vertexUniformBuffers[COMMAND_BUFFER_SETS];
			boost::container::vector<VkDeviceMemory> _vertexUniformBuffersMemory[COMMAND_BUFFER_SETS];
			boost::container::vector<VkBuffer> _fragmentUniformBuffers[COMMAND_BUFFER_SETS];
			boost::container::vector<VkDeviceMemory> _fragmentUniformBuffersMemory[COMMAND_BUFFER_SETS];
			boost::container::vector<VkBuffer> _fragmentDynamicUniformBuffers[COMMAND_BUFFER_SETS];
			boost::container::vector<VkDeviceMemory> _fragmentDynamicUniformBuffersMemory[COMMAND_BUFFER_SETS];

			// Staging-related members.
			VkBuffer _indexStagingBuffer;
			VkBuffer _vertexStagingBuffer;
			VkDeviceMemory _indexStagingMemory;
			VkDeviceMemory _vertexStagingMemory;

			// Vulkan configuration members.
			VkFormat _depthFormat;
			VkFormat _swapchainFormat;
			VkExtent2D _swapchainExtent;
			VkDeviceSize _minUniformBufferAlignment;

			// Vulkan synchronization members.
			boost::container::vector<VkSemaphore> _imageAvailable;
			boost::container::vector<VkSemaphore> _renderFinished;
			boost::container::vector<VkFence> _drawFences;

			// Entity-related members.
			boost::container::map<Core::Entity*, RenderInfo> _entitiesToRender;

			// Camera-related members.
			boost::shared_ptr<Entities::Camera> _activeCamera;

			// Light-related members.
			boost::shared_ptr<Entities::DirectionalLight> _directionalLight;
			boost::array<boost::shared_ptr<Entities::PointLight>, MAX_POINT_LIGHTS> _pointLights;
			boost::array<boost::shared_ptr<Entities::SpotLight>, MAX_SPOT_LIGHTS> _spotLights;

			#if _DEBUG
			VkDebugUtilsMessengerEXT _debugMessenger;
			#endif

			const Platform::Win32Window* _window;

		};
	}
}

#endif // PLATFORM_USE_VULKAN
