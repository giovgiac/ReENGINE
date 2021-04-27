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
#include "Graphics/Texture.hpp"
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
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/smart_ptr.hpp>
#include <boost/thread.hpp>
#include <utility>

#if PLATFORM_WINDOWS
#include "Platform/Win32/Window.hpp"
#define VK_USE_PLATFORM_WIN32_KHR
#endif

#include <vulkan/vulkan.h>

const usize MAX_FRAME_DRAWS		= 3;
const usize MAX_MATERIALS		= 8192;
const usize MAX_TEXTURES		= 4096;
const usize MAX_POINT_LIGHTS	= 32;
const usize MAX_SPOT_LIGHTS		= 32;

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

			// G-buffer pipeline.

			struct CameraUniform 
			{
				alignas(16)	Math::Matrix _projection;
			} _vertexUniform;

			struct MaterialUniform 
			{
				alignas(4)	f32 _specularPower;
				alignas(4)	f32 _specularStrength;
			};

			struct GBufferPush 
			{
				alignas(16)	Math::Matrix _view;
				alignas(16)	Math::Matrix _model;
			};

			// Lighting pipeline.

			struct LightingUniform
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

			struct LightingPush
			{
				alignas(16)	Math::Vector3 _eyePosition;
			};

			// Transfer-related structures.

            struct RenderableInfo
            {
				// Vertex-related information.
                VkBuffer _vertexBuffer;
                i32 _vertexCount;
				VkDeviceSize _vertexSize;

				// Index-related information.
                VkBuffer _indexBuffer;
                i32 _indexCount;
				VkDeviceSize _indexSize;

				// Texture-related information.
				VkImage _diffuseImage;

				// Descriptor-related information.
				Material* _material;
            };

			struct EntityInfo
			{
				boost::container::vector<RenderableInfo> _renderables;
				Components::TransformComponent* _transformComponent;
			};

			struct TransferInfo
			{
				Core::Entity* _entity;
				bool _isRemoval;
			};

			struct VertexInfo
			{
				VkBuffer _buffer;
				VkDeviceSize _size;
				boost::container::vector<Vertex> _vertices;
			};

			struct IndexInfo
			{
				VkBuffer _buffer;
				VkDeviceSize _size;
				boost::container::vector<u32> _indices;
			};

			struct TextureInfo
			{
				VkImage _image;
				VkDeviceSize _size;
				u32 _bpp;
				u32 _width;
				u32 _height;
				u8* _pixels;
			};

			// Vulkan-related structures.

			struct GBufferImage
			{
				VkImage _raw;
				VkImageView _view;
				VkDeviceMemory _memory;
			};

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
			usize GetAlignedSize(usize dataSize, usize dataAlignment) const;

			// Support create functions.
			RendererResult AllocateBuffer(VkBuffer buffer, VkMemoryPropertyFlags properties, VkDeviceMemory* outMemory);
			RendererResult AllocateImage(VkImage image, VkMemoryPropertyFlags properties, VkDeviceMemory* outMemory);
			RendererResult CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* outBuffer);
			RendererResult CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage* outImage);
			RendererResult CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags flags, VkImageView* outView) const;
			RendererResult CreateShaderModule(const boost::container::vector<char>& raw, VkShaderModule* outModule) const;
			RendererResult CreateIndexBuffer(boost::container::vector<u32>& indices, VkBuffer* outBuffer);
			RendererResult CreateVertexBuffer(boost::container::vector<Vertex>& vertices, VkBuffer* outBuffer);
			RendererResult CreateTextureImage(Texture* texture, VkImage* outImage);
			RendererResult CreateTextureImageView(VkImage image);
			RendererResult CreateTextureDescriptorSets();

			// Support destroy functions.
			void DestroyBuffer(VkBuffer buffer);
			void DestroyImage(VkImage image);

			// Support transfer functions.
			RendererResult ExecuteTransferOperations();
			RendererResult StageIndexBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory);
			RendererResult StageVertexBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory);
			RendererResult StageImageBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory);
			void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, u32 srcQueueFamily, u32 dstQueueFamily, VkImageLayout srcLayout, VkImageLayout dstLayout, 
				VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage);

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
			RendererResult CreateDescriptorSetLayouts();
			RendererResult CreatePushConstantRanges();
			RendererResult CreateGraphicsPipelines();
			RendererResult CreateGBuffer();
			RendererResult CreateFramebuffers();
			RendererResult CreateCommandPools();
			RendererResult CreateCommandBuffers();
			RendererResult CreateTextureSampler();
			RendererResult CreateUniformBuffers();
			RendererResult CreateDescriptorPools();
			RendererResult CreateDescriptorSets();
			RendererResult CreateSynchronization();

			// Record functions.
			RendererResult RecordCommands(usize offset, usize size);

			// Update functions.
			void UpdateDirectionalLight(LightingUniform::FragmentDirectionalLight* dstLight, const boost::shared_ptr<Entities::DirectionalLight>& srcLight);
			void UpdatePointLight(LightingUniform::FragmentPointLight* dstLight, const boost::shared_ptr<Entities::PointLight>& srcLight);
			void UpdateSpotLight(LightingUniform::FragmentSpotLight* dstLight, const boost::shared_ptr<Entities::SpotLight>& srcLight);
			void UpdateCameraUniformBuffers();
			void UpdateLightingUniformBuffers();
			void UpdateMaterialUniformBuffers();

			// Destroy functions.
			void DestroyCommandPools();
			void DestroyEntities();
			void DestroySwapchain();
			void DestroyDescriptorSetLayouts();
			void DestroyGraphicsPipelines();
			void DestroyGBuffer();
			void DestroyFramebuffers();
			void DestroyUniformBuffers();
			void DestroyDescriptorPools();
			void DestroySynchronization();

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
			u32 _graphicsQueueFamily;
			VkQueue _graphicsQueue;
			VkQueue _presentationQueue;
			VkSwapchainKHR _swapchain;
			boost::container::vector<SwapchainImage> _swapchainImages;
			boost::container::vector<VkFramebuffer> _swapchainFramebuffers;
			boost::container::vector<VkCommandBuffer> _commandBuffers;
			usize _currentFrame;

			// G-Buffer-related members.
			boost::container::vector<GBufferImage> _colorBuffer;
			boost::container::vector<GBufferImage> _depthBuffer;
			boost::container::vector<GBufferImage> _normalBuffer;
			boost::container::vector<GBufferImage> _positionBuffer;

			// Pipeline-related members.
			VkPipeline _gBufferPipeline;
			VkPipelineLayout _gBufferPipelineLayout;

			VkPipeline _lightingPipeline;
			VkPipelineLayout _lightingPipelineLayout;

			VkRenderPass _renderPass;
			VkCommandPool _graphicsPool;

			// Transfer-related members.
			u32 _transferQueueFamily;
			VkQueue _transferQueue;
			VkCommandPool _transferPool;
			VkCommandBuffer _transferBuffer;
			boost::bimap<VkBuffer, VkDeviceMemory> _bufferMemory;
			boost::bimap<VkImage, VkDeviceMemory> _imageMemory;
			boost::container::vector<VertexInfo> _vertexBuffersToTransfer;
			boost::container::vector<IndexInfo> _indexBuffersToTransfer;
			boost::container::vector<TextureInfo> _textureImagesToTransfer;
			boost::container::map<Core::Entity*, EntityInfo> _entitiesToTransfer;
			boost::lockfree::spsc_queue<VkImage> _releasedImages;
			boost::mutex _transferMutex;

			// Descriptor-related members.
			VkDescriptorSetLayout _bufferDescriptorSetLayout;
			VkDescriptorSetLayout _samplerDescriptorSetLayout;
			VkDescriptorSetLayout _gBufferDescriptorSetLayout;
			VkDescriptorSetLayout _lightingDescriptorSetLayout;
			VkDescriptorPool _bufferDescriptorPool;
			VkDescriptorPool _samplerDescriptorPool;
			VkDescriptorPool _gBufferDescriptorPool;
			VkDescriptorPool _lightingDescriptorPool;
			boost::container::vector<VkDescriptorSet> _bufferDescriptorSets;
			boost::container::vector<VkDescriptorSet> _gBufferDescriptorSets;
			boost::container::vector<VkDescriptorSet> _lightingDescriptorSets;
			VkPushConstantRange _gBufferPushConstantRange;
			VkPushConstantRange _lightingPushConstantRange;

			// Uniform buffer members.
			boost::container::vector<VkBuffer> _cameraUniformBuffers;
			boost::container::vector<VkDeviceMemory> _cameraUniformBuffersMemory;
			boost::container::vector<VkBuffer> _materialUniformBuffers;
			boost::container::vector<VkDeviceMemory> _materialUniformBuffersMemory;
			boost::container::vector<VkBuffer> _lightingUniformBuffers;
			boost::container::vector<VkDeviceMemory> _lightingUniformBuffersMemory;

			// Vulkan configuration members.
			VkFormat _colorFormat;
			VkFormat _depthFormat;
			VkFormat _normalFormat;
			VkFormat _positionFormat;
			VkFormat _swapchainFormat;
			VkExtent2D _swapchainExtent;
			VkDeviceSize _minUniformBufferAlignment;

			// Vulkan synchronization members.
			boost::container::vector<VkSemaphore> _imageAvailable;
			boost::container::vector<VkSemaphore> _renderFinished;
			boost::container::vector<VkFence> _drawFences;
			
			// Asset-related members.
			boost::bimap<Texture*, VkImage> _textureImage;
			boost::container::map<VkImage, VkImageView> _textureImageView;
			boost::container::map<VkImage, VkDescriptorSet> _textureDescriptorSets;
			boost::container::map<VkImage, usize> _textureReferences;
			VkSampler _textureSampler;

			// Entity-related members.
			boost::container::map<Core::Entity*, EntityInfo> _entitiesToRender;

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
