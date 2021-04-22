/*
 * Renderer.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Renderer.hpp"

#ifdef PLATFORM_USE_VULKAN

#include "Components/RenderComponent.hpp"
#include "Components/TransformComponent.hpp"
#include "Math/Color.hpp"
#include "Memory/Memory.hpp"

#include <boost/container/set.hpp>
#include <boost/foreach.hpp>
#include <boost/range/join.hpp>

#include <fstream>

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

static Re::Graphics::RendererResult ReadShader(const utf8* filename, boost::container::vector<char>* outShader) 
{
	std::ifstream file(filename, std::ios::binary | std::ios::ate);
	if (!file.is_open() || !outShader)
	{
		return Re::Graphics::RendererResult::Failure;
	}

	// Get file size and resize the output buffer.
	usize size = static_cast<usize>(file.tellg());
	outShader->resize(size);

	file.seekg(0);
	file.read(outShader->data(), size);
	file.close();

	return Re::Graphics::RendererResult::Success;
}

Re::Math::Color::operator VkClearColorValue() const
{
	return { Red, Green, Blue, Alpha };
}

namespace Re
{
	namespace Graphics
	{
		Renderer::Renderer()
			: _currentBuffer(0), _currentFrame(0), _streamingQueue(128), _streamingThreadShouldClose(false),
			_indexStagingBuffer(VK_NULL_HANDLE), _vertexStagingBuffer(VK_NULL_HANDLE), 
			_indexStagingMemory(VK_NULL_HANDLE), _vertexStagingMemory(VK_NULL_HANDLE)
		{}

		bool Renderer::AddEntity(Core::Entity* entity)
		{
			TransferInfo info = {};
			info._entity = entity;
			info._isRemoval = false;

			bool pushed = _streamingQueue.push(info);
			_streamingRequested.notify_one();
			return pushed;
		}

		bool Renderer::RemoveEntity(Core::Entity* entityToRemove)
		{
			TransferInfo info = {};
			info._entity = entityToRemove;
			info._isRemoval = true;

			bool pushed = _streamingQueue.push(info);
			_streamingRequested.notify_one();
			return pushed;
		}

		RendererResult Renderer::Render()
		{
			// Enforce maximum number of drawable frames with fences.
			CHECK_RESULT(vkWaitForFences(_device._logical, 1, &_drawFences[_currentFrame], VK_TRUE, -1), VK_SUCCESS, RendererResult::Failure)
			CHECK_RESULT(vkResetFences(_device._logical, 1, &_drawFences[_currentFrame]), VK_SUCCESS, RendererResult::Failure)

			// Get next image to render to (and signal when succeeded).
			u32 imageIndex;
			CHECK_RESULT(vkAcquireNextImageKHR(_device._logical, _swapchain, -1, _imageAvailable[_currentFrame], VK_NULL_HANDLE, &imageIndex), VK_SUCCESS, RendererResult::Failure)
			
			// Load the value of the current set of command buffers to use.
			usize current = _currentBuffer.load();

			// Re-write the command buffers to update values, if not already rerecording.
			CHECK_RESULT(RecordCommands(current, imageIndex, 1), RendererResult::Success, RendererResult::Failure);

			// Define stages that we need to wait for semaphores.
			VkPipelineStageFlags waitStages[] = {
				VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
			};

			// Submit appropriate command buffer to acquired image.
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.waitSemaphoreCount = 1;
			submitInfo.pWaitSemaphores = &_imageAvailable[_currentFrame];
			submitInfo.pWaitDstStageMask = waitStages;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &_commandBuffers[current][imageIndex];
			submitInfo.signalSemaphoreCount = 1;
			submitInfo.pSignalSemaphores = &_renderFinished[_currentFrame];
			
			CHECK_RESULT(vkQueueSubmit(_graphicsQueue, 1, &submitInfo, _drawFences[_currentFrame]), VK_SUCCESS, RendererResult::Failure)

			// Present rendered image to screen.
			VkPresentInfoKHR presentInfo = {};
			presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			presentInfo.waitSemaphoreCount = 1;
			presentInfo.pWaitSemaphores = &_renderFinished[_currentFrame];
			presentInfo.swapchainCount = 1;
			presentInfo.pSwapchains = &_swapchain;
			presentInfo.pImageIndices = &imageIndex;
			
			CHECK_RESULT(vkQueuePresentKHR(_presentationQueue, &presentInfo), VK_SUCCESS, RendererResult::Failure)

			_currentFrame = (_currentFrame + 1) % MAX_FRAME_DRAWS;
			return RendererResult::Success;
		}

		RendererResult Renderer::Startup(const Platform::Win32Window& window)
		{
			// Store window handle
			_window = &window;

			// Vulkan startup
			CHECK_RESULT_WITH_ERROR(CreateInstance(), RendererResult::Success, NTEXT("Failed to create instance!\n"), RendererResult::Failure)
			#if _DEBUG
			CHECK_RESULT_WITH_ERROR(CreateDebugCallback(), RendererResult::Success, NTEXT("Failed to create debug callback!\n"), RendererResult::Failure)
			#endif
			#if PLATFORM_WINDOWS
			CHECK_RESULT_WITH_ERROR(CreateWindowsSurface(window), RendererResult::Success, NTEXT("Failed to create surface in Windows!\n"), RendererResult::Failure)
			#endif
			CHECK_RESULT_WITH_ERROR(RetrievePhysicalDevice(), RendererResult::Success, NTEXT("Failed to retrieve appropriate physical device!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateLogicalDevice(), RendererResult::Success, NTEXT("Failed to create logical device!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateSwapchain(), RendererResult::Success, NTEXT("Failed to create swapchain!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateDepthBufferImage(), RendererResult::Success, NTEXT("Failed to create depth buffer image!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateRenderPass(), RendererResult::Success, NTEXT("Failed to create renderpass!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateDescriptorSetLayout(), RendererResult::Success, NTEXT("Failed to create descriptor set layout!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreatePushConstantRanges(), RendererResult::Success, NTEXT("Failed to create push constant range!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateGraphicsPipeline(), RendererResult::Success, NTEXT("Failed to create graphics pipeline!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateFramebuffers(), RendererResult::Success, NTEXT("Failed to create framebuffers!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateCommandPools(), RendererResult::Success, NTEXT("Failed to create command pools!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateCommandBuffers(), RendererResult::Success, NTEXT("Failed to create command buffers!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateUniformBuffers(), RendererResult::Success, NTEXT("Failed to create uniform buffers!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateDescriptorPool(), RendererResult::Success, NTEXT("Failed to create descriptor pool!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateDescriptorSets(), RendererResult::Success, NTEXT("Failed to create descriptor sets!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateSynchronization(), RendererResult::Success, NTEXT("Failed to create synchronization!\n"), RendererResult::Failure)

			// Transfer thread startup
			_streamingThreadShouldClose.store(false, boost::memory_order_release);
			_streamingThread = boost::thread(boost::bind(&Renderer::EntityStreaming, this));

			return RendererResult::Success;
		}

		void Renderer::Shutdown()
		{
			// Transfer thread shutdown
			_streamingThreadShouldClose.store(true, boost::memory_order_release);
			_streamingRequested.notify_all();
			_streamingThread.join();

			// Vulkan shutdown
			vkDeviceWaitIdle(_device._logical);
			
			DestroyEntities();
			DestroySynchronization();
			vkDestroyDescriptorPool(_device._logical, _descriptorPool, nullptr);
			vkDestroyDescriptorSetLayout(_device._logical, _descriptorSetLayout, nullptr);
			DestroyUniformBuffers();
			DestroyCommandPools();
			DestroyFramebuffers();
			vkDestroyPipeline(_device._logical, _graphicsPipeline, nullptr);
			vkDestroyPipelineLayout(_device._logical, _pipelineLayout, nullptr);
			vkDestroyRenderPass(_device._logical, _renderPass, nullptr);
			DestroyDepthBufferImage();
			DestroySwapchain();
			vkDestroyDevice(_device._logical, nullptr);
			vkDestroySurfaceKHR(_instance, _surface, nullptr);
			#if _DEBUG
			DestroyDebugCallback();
			#endif
			vkDestroyInstance(_instance, nullptr);
		}

		RendererResult Renderer::ActivateLight(const boost::shared_ptr<Entities::DirectionalLight>& light)
		{
			// Load current command buffer.
			usize current = _currentBuffer.load();

			// Retrieve configuration values from the selected light.
			_fragmentUniform._directionalLight._base._color = light->GetColor();
			_fragmentUniform._directionalLight._base._ambientStrength = light->GetAmbientStrength();
			_fragmentUniform._directionalLight._base._diffuseStrength = light->GetDiffuseStrength();
			_fragmentUniform._directionalLight._direction = light->GetDirection();

			// Update the fragment uniform buffer values.
			UpdateFragmentUniformBuffers(current);

			// Store specified light as active light.
			_directionalLight = light;
			return RendererResult::Success;
		}

		RendererResult Renderer::ActivateLight(const boost::shared_ptr<Entities::PointLight>& light)
		{
			// Load current command buffer.
			usize current = _currentBuffer.load();

			// Update the fragment uniform with the point lights.
			usize availableIndex = 0;
			usize lightCount = 0;
			bool inserted = false;
			for (usize i = 0; i < _pointLights.size(); ++i)
			{
				if (_pointLights[i])
				{
					UpdatePointLight(&_fragmentUniform._pointLights[i], _pointLights[i]);
					lightCount++;
				}
				else
				{
					// Insert element in available position, if not inserted.
					if (!inserted)
					{
						UpdatePointLight(&_fragmentUniform._pointLights[i], light);
						availableIndex = i;
						lightCount++;
						inserted = true;
					}
				}
			}

			// If failed to find slot, return with failure.
			if (!inserted) return RendererResult::Failure;

			// Update light count value in the uniform.
			_fragmentUniform._pointLightCount = lightCount;

			// Update the fragment uniform buffer values.
			UpdateFragmentUniformBuffers(current);

			// Add new light to the list of point lights.
			_pointLights[availableIndex] = light;
			return RendererResult::Success;
		}

		RendererResult Renderer::ActivateLight(const boost::shared_ptr<Entities::SpotLight>& light)
		{
			// Load current command buffer.
			usize current = _currentBuffer.load();

			// Update the fragment uniform with the spot lights.
			usize availableIndex = 0;
			usize lightCount = 0;
			bool inserted = false;
			for (usize i = 0; i < _spotLights.size(); ++i)
			{
				if (_spotLights[i])
				{
					UpdateSpotLight(&_fragmentUniform._spotLights[i], _spotLights[i]);
					lightCount++;
				}
				else
				{
					// Insert element in available position, if not inserted.
					if (!inserted)
					{
						UpdateSpotLight(&_fragmentUniform._spotLights[i], light);
						availableIndex = i;
						lightCount++;
						inserted = true;
					}
				}
			}

			// If failed to find slot, return with failure.
			if (!inserted) return RendererResult::Failure;

			// Update light count value in the uniform.
			_fragmentUniform._spotLightCount = lightCount;

			// Update the fragment uniform buffer values.
			UpdateFragmentUniformBuffers(current);

			// Add new light to the list of spot lights.
			_spotLights[availableIndex] = light;
			_spotLights[availableIndex]->OnParameterChanged.connect([=]() {
				// Load current command buffer.
				usize buffer = _currentBuffer.load();

				UpdateSpotLight(&_fragmentUniform._spotLights[availableIndex], _spotLights[availableIndex]);
				UpdateFragmentUniformBuffers(buffer);
			});
			return RendererResult::Success;
		}

		void Renderer::DeactivateLight(const boost::shared_ptr<Entities::PointLight>& light)
		{
			// Load current command buffer.
			usize current = _currentBuffer.load();

			for (usize i = 0; i < _pointLights.size(); ++i)
			{
				if (_pointLights[i] == light)
				{
					// Reset fragment uniform values to default.
					_fragmentUniform._pointLights[i] = {};

					// Remove point light reference from array.
					_pointLights[i] = nullptr;
				}
			}

			// Update the fragment uniform buffer values.
			UpdateFragmentUniformBuffers(current);
		}

		void Renderer::DeactivateLight(const boost::shared_ptr<Entities::SpotLight>& light)
		{
			// Load current command buffer.
			usize current = _currentBuffer.load();

			for (usize i = 0; i < _spotLights.size(); ++i)
			{
				if (_spotLights[i] == light)
				{
					// Reset fragment uniform values to default.
					_fragmentUniform._spotLights[i] = {};

					// Remove spot light reference from array.
					_spotLights[i] = nullptr;
				}
			}

			// Update the fragment uniform buffer values.
			UpdateFragmentUniformBuffers(current);
		}

		void Renderer::SetActiveCamera(const boost::shared_ptr<Entities::Camera>& newCamera)
		{
			// Load current command buffer.
			usize current = _currentBuffer.load();

			// Retrieve projection matrix from the selected camera.
			_vertexUniform._projection = newCamera->GetProjection(static_cast<f32>(_swapchainExtent.width) / static_cast<f32>(_swapchainExtent.height));
			
			// Update the uniform buffer values.
			UpdateVertexUniformBuffers(current);

			// Store specified camera as active camera.
			_activeCamera = newCamera;
		}

		void Renderer::EntityStreaming()
		{
			while (true)
			{
				boost::unique_lock<boost::mutex> lock(_streamingMutex);
				_streamingRequested.wait(lock, [this] {
					return _streamingThreadShouldClose.load(boost::memory_order_acquire) || !_streamingQueue.empty(); 
				});

				if (_streamingThreadShouldClose.load(boost::memory_order_acquire))
				{
					return;
				}
				else
				{
					// Perform transfer operations that were requested.
					TransferInfo transferInfo;
					while (_streamingQueue.pop(transferInfo))
					{
						if (!transferInfo._isRemoval)
						{
							if (transferInfo._entity->HasComponent<Components::RenderComponent>())
							{
								auto renderComponent = transferInfo._entity->GetComponent<Components::RenderComponent>();
								if (renderComponent)
								{
									auto indices = renderComponent->GetIndices();
									auto vertices = renderComponent->GetVertices();
									if (indices.size() == 0 || vertices.size() == 0) continue;

									// Create rendering information for renderable entity.
									RenderInfo renderInfo = {};
									renderInfo._indexCount = static_cast<i32>(indices.size());
									renderInfo._vertexCount = static_cast<i32>(vertices.size());
									renderInfo._material = renderComponent->GetMaterial();

									// Create required buffers and images for the rendering.
									CreateIndexBuffer(indices, &renderInfo._indexBuffer);
									CreateVertexBuffer(vertices, &renderInfo._vertexBuffer);

									// Record the model matrix of the entity.
									if (transferInfo._entity->HasComponent<Components::TransformComponent>())
									{
										auto transformComponent = transferInfo._entity->GetComponent<Components::TransformComponent>();
										if (transformComponent)
										{
											renderInfo._transformComponent = transformComponent;
										}
									}

									_entitiesToTransfer.emplace_unique(transferInfo._entity, renderInfo);
								}
							}
						}
						else
						{
							auto entity = _entitiesToRender.find(transferInfo._entity);
							if (entity != _entitiesToRender.end())
							{
								// Destroy buffers and images created for the rendering.
								DestroyBuffer(entity->second._indexBuffer);
								DestroyBuffer(entity->second._vertexBuffer);

								_entitiesToRender.erase(entity);
							}
						}
					}

					// Execute pending transfer operations.
					ExecuteTransferOperations();
				}
			}
		}

		RendererResult Renderer::RetrievePhysicalDevice()
		{
			u32 deviceCount = 0;
			CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr), VK_SUCCESS, RendererResult::Failure)

			// If there aren't any devices available, then Vulkan is not supported.
			if (deviceCount == 0)
			{
				return RendererResult::Failure;
			}

			boost::container::vector<VkPhysicalDevice> devices(deviceCount);
			CHECK_RESULT(vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data()), VK_SUCCESS, RendererResult::Failure)

			// Pick an appropriate physical device from the available list.
			for (auto& dev : devices)
			{
				if (CheckPhysicalDeviceSuitable(dev))
				{
					// Store suitable physical device as the chosen one.
					_device._physical = dev;

					// Retrieve properties of selected physical device.
					VkPhysicalDeviceProperties deviceProperties;
					vkGetPhysicalDeviceProperties(dev, &deviceProperties);

					// Store physical device configurations for later use.
					_minUniformBufferAlignment = deviceProperties.limits.minUniformBufferOffsetAlignment;

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

		u32 Renderer::FindMemoryTypeIndex(u32 allowedTypes, VkMemoryPropertyFlags flags) const
		{
			VkPhysicalDeviceMemoryProperties memoryProperties;
			vkGetPhysicalDeviceMemoryProperties(_device._physical, &memoryProperties);

			for (u32 i = 0; i < memoryProperties.memoryTypeCount; ++i)
			{
				// Find a memory type that is allowed and has the desired property flags.
				if ((allowedTypes & (1 << i)) && ((memoryProperties.memoryTypes[i].propertyFlags & flags) == flags))
				{
					return i;
				}
			}

			return -1;
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
					familyInfo._graphicsFamily = i;
				}

				// Check if queue family supports presentation.
				VkBool32 presentationSupport = false;
				vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentationSupport);
				if (familyProperties[i].queueCount > 0 && presentationSupport)
				{
					familyInfo._presentationFamily = i;
				}

				// Check for a dedicated transfer family that can't do graphical operations.
				if (familyProperties[i].queueCount > 0 &&
					(familyProperties[i].queueFlags & VK_QUEUE_TRANSFER_BIT && !(familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT)))
				{
					familyInfo._transferFamily = i;
				}

				if (familyInfo.IsValid())
				{
					break;
				}
			}

			// Fallback to using graphics as transfer family if no dedicated family is found.
			if (familyInfo._graphicsFamily != -1 && familyInfo._transferFamily == -1)
			{
				familyInfo._transferFamily == familyInfo._graphicsFamily;
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

		usize Renderer::GetUniformAlignment(usize dataSize) const
		{
			return (dataSize + _minUniformBufferAlignment - 1) & ~(_minUniformBufferAlignment - 1);
		}

		RendererResult Renderer::AllocateBuffer(VkBuffer buffer, VkMemoryPropertyFlags properties, VkDeviceMemory* outMemory)
		{
			if (!outMemory) return RendererResult::Failure;

			// Get buffer memory requirements.
			VkMemoryRequirements requirements;
			vkGetBufferMemoryRequirements(_device._logical, buffer, &requirements);

			// Allocate memory to buffer.
			VkMemoryAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo.allocationSize = requirements.size;
			allocateInfo.memoryTypeIndex = FindMemoryTypeIndex(requirements.memoryTypeBits, properties);

			CHECK_RESULT(vkAllocateMemory(_device._logical, &allocateInfo, nullptr, outMemory), VK_SUCCESS, RendererResult::Failure)
			CHECK_RESULT(vkBindBufferMemory(_device._logical, buffer, *outMemory, 0), VK_SUCCESS, RendererResult::Failure);
			return RendererResult::Success;
		}

		RendererResult Renderer::AllocateImage(VkImage image, VkMemoryPropertyFlags properties, VkDeviceMemory* outMemory)
		{
			if (!outMemory) return RendererResult::Failure;
			
			// Retrieve memory requirements for the created image.
			VkMemoryRequirements memoryRequirements = {};
			vkGetImageMemoryRequirements(_device._logical, image, &memoryRequirements);

			// Allocate memory to image.
			VkMemoryAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			allocateInfo.allocationSize = memoryRequirements.size;
			allocateInfo.memoryTypeIndex = FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, properties);

			CHECK_RESULT(vkAllocateMemory(_device._logical, &allocateInfo, nullptr, outMemory), VK_SUCCESS, RendererResult::Failure);
			CHECK_RESULT(vkBindImageMemory(_device._logical, image, *outMemory, 0), VK_SUCCESS, RendererResult::Failure);
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkBuffer* outBuffer)
		{
			if (!outBuffer) return RendererResult::Failure;
			if (size <= 0) return RendererResult::Failure;

			VkBufferCreateInfo bufferInfo = {};
			bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
			bufferInfo.size = size;
			bufferInfo.usage = usage;
			bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			CHECK_RESULT(vkCreateBuffer(_device._logical, &bufferInfo, nullptr, outBuffer), VK_SUCCESS, RendererResult::Failure)
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateImage(u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkImage* outImage)
		{
			if (!outImage) return RendererResult::Failure;

			// Configure image that will be created.
			VkImageCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
			createInfo.imageType = VK_IMAGE_TYPE_2D;
			createInfo.extent.width = width;
			createInfo.extent.height = height;
			createInfo.extent.depth = 1;
			createInfo.mipLevels = 1;
			createInfo.arrayLayers = 1;
			createInfo.format = format;
			createInfo.tiling = tiling;
			createInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			createInfo.usage = usage;
			createInfo.samples = VK_SAMPLE_COUNT_1_BIT;
			createInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

			CHECK_RESULT(vkCreateImage(_device._logical, &createInfo, nullptr, outImage), VK_SUCCESS, RendererResult::Failure);
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags flags, VkImageView* outView) const
		{
			if (!outView) return RendererResult::Failure;
			
			VkImageViewCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
			createInfo.image = image;
			createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
			createInfo.format = format;

			// Component swizzling.
			createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
			createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;

			// Subresources that control the part you view of an image.
			createInfo.subresourceRange.aspectMask = flags;
			createInfo.subresourceRange.baseMipLevel = 0;
			createInfo.subresourceRange.levelCount = 1;
			createInfo.subresourceRange.baseArrayLayer = 0;
			createInfo.subresourceRange.layerCount = 1;

			CHECK_RESULT(vkCreateImageView(_device._logical, &createInfo, nullptr, outView), VK_SUCCESS, RendererResult::Failure)
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateShaderModule(const boost::container::vector<char>& raw, VkShaderModule* outModule) const
		{
			if (!outModule) return RendererResult::Failure;
			
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = raw.size();
			createInfo.pCode = reinterpret_cast<const u32*>(raw.data());

			CHECK_RESULT(vkCreateShaderModule(_device._logical, &createInfo, nullptr, outModule), VK_SUCCESS, RendererResult::Failure)
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateIndexBuffer(boost::container::vector<u32>& indices, VkBuffer* outBuffer)
		{
			if (!outBuffer) return RendererResult::Failure;

			// Create the actual index buffer located in the GPU.
			VkDeviceSize size = sizeof(u32) * indices.size();
			CHECK_RESULT(CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, outBuffer), RendererResult::Success, RendererResult::Failure)

			// Create index information to be able to transfer.
			IndexInfo indexInfo = {};
			indexInfo._buffer = *outBuffer;
			indexInfo._size = 0;
			indexInfo._indices = indices;

			_indexBuffersToTransfer.emplace_back(indexInfo);
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateVertexBuffer(boost::container::vector<Vertex>& vertices, VkBuffer* outBuffer)
		{
			if (!outBuffer) return RendererResult::Failure;

			// Create the actual vertex buffer located in the GPU.
			VkDeviceSize size = sizeof(Vertex) * vertices.size();
			CHECK_RESULT(CreateBuffer(size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, outBuffer), RendererResult::Success, RendererResult::Failure)

			// Create vertex information to be able to transfer.
			VertexInfo vertexInfo = {};
			vertexInfo._buffer = *outBuffer;
			vertexInfo._size = 0;
			vertexInfo._vertices = vertices;

			_vertexBuffersToTransfer.emplace_back(vertexInfo);
			return RendererResult::Success;
		}

		void Renderer::DestroyBuffer(VkBuffer buffer)
		{
			if (_bufferMemory.left.find(buffer) != _bufferMemory.left.end())
			{
				VkDeviceMemory memory = _bufferMemory.left.at(buffer);
				usize distance = std::distance(_bufferMemory.right.lower_bound(memory), _bufferMemory.right.upper_bound(memory));
				
				vkDestroyBuffer(_device._logical, buffer, nullptr);
				_bufferMemory.left.erase(buffer);

				if (distance <= 1)
				{
					vkFreeMemory(_device._logical, memory, nullptr);
					_bufferMemory.right.erase(memory);
				}
			}
			else
			{
				vkDestroyBuffer(_device._logical, buffer, nullptr);
				_bufferMemory.left.erase(buffer);
			}
		}

		void Renderer::CleanStageBuffers()
		{
			if (_indexStagingBuffer && _indexStagingMemory)
			{
				vkDestroyBuffer(_device._logical, _indexStagingBuffer, nullptr);
				vkFreeMemory(_device._logical, _indexStagingMemory, nullptr);
			}

			if (_vertexStagingBuffer && _vertexStagingMemory)
			{
				vkDestroyBuffer(_device._logical, _vertexStagingBuffer, nullptr);
				vkFreeMemory(_device._logical, _vertexStagingMemory, nullptr);
			}
		}

		RendererResult Renderer::ExecuteTransferOperations()
		{
			// Calculate proper memory size and type for index and vertex buffers.
			//usize indexAllocationSize = 0, vertexAllocationSize = 0;
			//u32 indexAllowedTypes = 0, vertexAllowedTypes = 0;
			//for (auto& entity : _entitiesToTransfer)
			//{
			//	auto& renderInfo = entity.second;
			//	VkMemoryRequirements indexMemoryRequirements;
			//	VkMemoryRequirements vertexMemoryRequirements;

			//	// Get buffer memory requirements.
			//	vkGetBufferMemoryRequirements(_device._logical, renderInfo._indexBuffer, &indexMemoryRequirements);
			//	vkGetBufferMemoryRequirements(_device._logical, renderInfo._vertexBuffer, &vertexMemoryRequirements);

			//	// Update size of individual buffer.
			//	renderInfo._indexSize = indexMemoryRequirements.size;
			//	renderInfo._vertexSize = vertexMemoryRequirements.size;

			//	// Update general values for big chunk of memory.
			//	indexAllocationSize += indexMemoryRequirements.size;
			//	indexAllowedTypes |= indexMemoryRequirements.memoryTypeBits;
			//	vertexAllocationSize += vertexMemoryRequirements.size;
			//	vertexAllowedTypes |= vertexMemoryRequirements.memoryTypeBits;
			//}

			// Calculate proper memory size and type for vertex buffer.
			usize vertexAllocationSize = 0;
			u32 vertexAllowedTypes = 0;
			for (auto& vInfo : _vertexBuffersToTransfer)
			{
				// Get buffer memory requirements.
				VkMemoryRequirements memoryRequirements;
				vkGetBufferMemoryRequirements(_device._logical, vInfo._buffer, &memoryRequirements);

				// Update size of individual buffer.
				vInfo._size = memoryRequirements.size;

				// Update general values for big chunk of memory.
				vertexAllocationSize += memoryRequirements.size;
				vertexAllowedTypes |= memoryRequirements.memoryTypeBits;
			}

			// Calculate proper memory size and type for index buffer.
			usize indexAllocationSize = 0;
			u32 indexAllowedTypes = 0;
			for (auto& iInfo : _indexBuffersToTransfer)
			{
				// Get buffer memory requirements.
				VkMemoryRequirements memoryRequirements;
				vkGetBufferMemoryRequirements(_device._logical, iInfo._buffer, &memoryRequirements);

				// Update size of individual buffer.
				iInfo._size = memoryRequirements.size;

				// Update general values for big chunk of memory.
				indexAllocationSize += memoryRequirements.size;
				indexAllowedTypes |= memoryRequirements.memoryTypeBits;
			}

			// Create staging buffers, allocate memory and copy data over to them.
			u32 i = sizeof(Vertex);
			//CleanStageBuffers();
			CHECK_RESULT(StageVertexBuffer(vertexAllocationSize, &_vertexStagingBuffer, &_vertexStagingMemory), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(StageIndexBuffer(indexAllocationSize, &_indexStagingBuffer, &_indexStagingMemory), RendererResult::Success, RendererResult::Failure)
			
			// Allocate memory to real vertex buffer.
			VkDeviceMemory vertexMemory;
			VkMemoryAllocateInfo vertexAllocateInfo = {};
			vertexAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			vertexAllocateInfo.allocationSize = vertexAllocationSize;
			vertexAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(vertexAllowedTypes, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			CHECK_RESULT(vkAllocateMemory(_device._logical, &vertexAllocateInfo, nullptr, &vertexMemory), VK_SUCCESS, RendererResult::Failure)
			
			// Allocate memory to real index buffer.
			VkDeviceMemory indexMemory;
			VkMemoryAllocateInfo indexAllocateInfo = {};
			indexAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
			indexAllocateInfo.allocationSize = indexAllocationSize;
			indexAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(indexAllowedTypes, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

			CHECK_RESULT(vkAllocateMemory(_device._logical, &indexAllocateInfo, nullptr, &indexMemory), VK_SUCCESS, RendererResult::Failure)

			// Bind memory to real buffers.
			//VkDeviceSize indexOffset = 0, vertexOffset = 0;
			//for (const auto& entity : _entitiesToTransfer)
			//{
			//	const auto& renderInfo = entity.second;

			//	// Bind memory to index buffer.
			//	CHECK_RESULT(vkBindBufferMemory(_device._logical, renderInfo._indexBuffer, indexMemory, indexOffset), VK_SUCCESS, RendererResult::Failure)

			//	// Store index memory into bimap for tracking and increase offset.
			//	_bufferMemory.insert(boost::bimap<VkBuffer, VkDeviceMemory>::value_type(renderInfo._indexBuffer, indexMemory));
			//	indexOffset += renderInfo._indexSize;

			//	// Bind memory to vertex buffer.
			//	CHECK_RESULT(vkBindBufferMemory(_device._logical, renderInfo._vertexBuffer, vertexMemory, vertexOffset), VK_SUCCESS, RendererResult::Failure)

			//	// Store vertex memory into bimap for tracking and increase offset.
			//	_bufferMemory.insert(boost::bimap<VkBuffer, VkDeviceMemory>::value_type(renderInfo._vertexBuffer, vertexMemory));
			//	vertexOffset += renderInfo._vertexSize;
			//}

			// Bind memory to real vertex buffer.
			VkDeviceSize vertexOffset = 0;
			for (const auto& vInfo : _vertexBuffersToTransfer)
			{
				CHECK_RESULT(vkBindBufferMemory(_device._logical, vInfo._buffer, vertexMemory, vertexOffset), VK_SUCCESS, RendererResult::Failure)
				
				_bufferMemory.insert(boost::bimap<VkBuffer, VkDeviceMemory>::value_type(vInfo._buffer, vertexMemory));
				vertexOffset += vInfo._size;
			}

			// Bind memory to real index buffer.
			VkDeviceSize indexOffset = 0;
			for (const auto& iInfo : _indexBuffersToTransfer)
			{
				CHECK_RESULT(vkBindBufferMemory(_device._logical, iInfo._buffer, indexMemory, indexOffset), VK_SUCCESS, RendererResult::Failure)
				
				_bufferMemory.insert(boost::bimap<VkBuffer, VkDeviceMemory>::value_type(iInfo._buffer, indexMemory));
				indexOffset += iInfo._size;
			}

			// Begin command buffer as optimized for one-time usage.
			VkCommandBufferBeginInfo beginInfo = {};
			beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			// Start recording commands for the transfer buffer.
			CHECK_RESULT(vkResetCommandBuffer(_transferBuffer, 0), VK_SUCCESS, RendererResult::Failure)
			CHECK_RESULT(vkBeginCommandBuffer(_transferBuffer, &beginInfo), VK_SUCCESS, RendererResult::Failure)

			// Copy data from staging buffers to real buffers.
			//VkDeviceSize indexSrcOffset = 0, vertexSrcOffset = 0;
			//for (const auto& entity : _entitiesToTransfer)
			//{
			//	const auto& renderInfo = entity.second;
			//	VkDeviceSize iSize = renderInfo._indexCount * sizeof(u32);
			//	VkDeviceSize vSize = renderInfo._vertexCount * sizeof(Vertex);
			//}

			VkDeviceSize vertexSrcOffset = 0;
			for (const auto& vInfo : _vertexBuffersToTransfer)
			{
				VkDeviceSize size = vInfo._vertices.size() * sizeof(Vertex);

				// Configure region of buffers to copy from.
				VkBufferCopy region = {};
				region.srcOffset = vertexSrcOffset;
				region.dstOffset = 0;
				region.size = size;

				vkCmdCopyBuffer(_transferBuffer, _vertexStagingBuffer, vInfo._buffer, 1, &region);
				vertexSrcOffset += vInfo._size;
			}

			VkDeviceSize indexSrcOffset = 0;
			for (const auto& iInfo : _indexBuffersToTransfer)
			{
				VkDeviceSize size = iInfo._indices.size() * sizeof(u32);

				// Configure region of buffers to copy from.
				VkBufferCopy region = {};
				region.srcOffset = indexSrcOffset;
				region.dstOffset = 0;
				region.size = size;

				vkCmdCopyBuffer(_transferBuffer, _indexStagingBuffer, iInfo._buffer, 1, &region);
				indexSrcOffset += iInfo._size;
			}

			CHECK_RESULT(vkEndCommandBuffer(_transferBuffer), VK_SUCCESS, RendererResult::Failure)

			// Configure queue submission information.
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &_transferBuffer;

			// Load next inactive command buffer.
			usize current = _currentBuffer.load();

			// Submit command to queue and wait until it finishes.
			CHECK_RESULT(vkQueueSubmit(_transferQueue, 1, &submitInfo, nullptr), VK_SUCCESS, RendererResult::Failure)

			// TODO: In the future, record secondary command buffers for static objects.
			
			// Re-record the commands after performing transfering operations.
			// RecordCommands(inactive, 0, _commandBuffers[inactive].size(), true);
			UpdateVertexUniformBuffers(current);
			UpdateFragmentUniformBuffers(current);
			UpdateFragmentDynamicUniformBuffers(current);

			// Wait until the transfer operations are completed before proceeding.
			CHECK_RESULT(vkQueueWaitIdle(_transferQueue), VK_SUCCESS, RendererResult::Failure);

			// Add new entities to renderable entities.
			_entitiesToRender.insert_unique(_entitiesToTransfer.begin(), _entitiesToTransfer.end());

			// Cleanup transfered buffers and entities.
			CleanStageBuffers();
			_vertexBuffersToTransfer.clear();
			_indexBuffersToTransfer.clear();
			_entitiesToTransfer.clear();

			return RendererResult::Success;
		}

		RendererResult Renderer::StageIndexBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory)
		{
			// Create staging buffer.
			CHECK_RESULT(CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(AllocateBuffer(*stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemory), RendererResult::Success, RendererResult::Failure)

			// Map pointer to staging buffer location and transfer data.
			void* data;
			CHECK_RESULT(vkMapMemory(_device._logical, *stagingMemory, 0, bufferSize, 0, &data), VK_SUCCESS, RendererResult::Failure)

			VkDeviceSize stagingOffset = 0;
			for (const auto& iInfo : _indexBuffersToTransfer)
			{
				Memory::NMemCpy((u8*)data + stagingOffset, iInfo._indices.data(), (usize)(iInfo._indices.size() * sizeof(u32)));
				stagingOffset += iInfo._size;
			}

			vkUnmapMemory(_device._logical, *stagingMemory);
			return RendererResult::Success;
		}

		RendererResult Renderer::StageVertexBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory)
		{
			// Create staging buffer.
			CHECK_RESULT(CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(AllocateBuffer(*stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemory), RendererResult::Success, RendererResult::Failure)

			// Map pointer to staging buffer location and transfer data.
			void* data;
			CHECK_RESULT(vkMapMemory(_device._logical, *stagingMemory, 0, bufferSize, 0, &data), VK_SUCCESS, RendererResult::Failure)

			VkDeviceSize stagingOffset = 0;
			for (const auto& vInfo : _vertexBuffersToTransfer)
			{
				Memory::NMemCpy((u8*)data + stagingOffset, vInfo._vertices.data(), (usize)(vInfo._vertices.size() * sizeof(Vertex)));
				stagingOffset += vInfo._size;
			}

			vkUnmapMemory(_device._logical, *stagingMemory);
			return RendererResult::Success;
		}

		VkSurfaceFormatKHR Renderer::ChooseBestSurfaceFormat(const boost::container::vector<VkSurfaceFormatKHR>& formats) const
		{
			// If only one format available, and it is the UNDEFINED format, then all formats are available.
			if (formats.size() == 1 && formats[0].format == VK_FORMAT_UNDEFINED)
			{
				return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
			}

			// Otherwise, loop over all formats and find the most suitable.
			for (const auto& fmt : formats)
			{
				if ((fmt.format == VK_FORMAT_B8G8R8A8_UNORM || fmt.format == VK_FORMAT_R8G8B8A8_UNORM) && 
					(fmt.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR))
				{
					return fmt;
				}
			}

			// If not found, return first format and hope that it all works out.
			return formats[0];
		}

		VkPresentModeKHR Renderer::ChooseBestPresentationMode(const boost::container::vector<VkPresentModeKHR>& modes) const
		{
			// Look for the ideal presentation mode in the list of available modes.
			for (const auto& pm : modes)
			{
				if (pm == VK_PRESENT_MODE_MAILBOX_KHR)
				{
					return pm;
				}
			}

			// If not found, return the FIFO mode that is guaranteed to be present by the Vulkan Spec.
			return VK_PRESENT_MODE_FIFO_KHR;
		}

		VkFormat Renderer::ChooseBestSupportedFormat(const boost::container::vector<VkFormat>& formats, VkImageTiling tiling, VkFormatFeatureFlags features) const
		{
			// Go through options and find a compatible match.
			for (auto format : formats)
			{
				// Retrieve the properties of the format in the physical device.
				VkFormatProperties properties;
				vkGetPhysicalDeviceFormatProperties(_device._physical, format, &properties);

				if (tiling == VK_IMAGE_TILING_LINEAR && (properties.linearTilingFeatures & features) == features)
				{
					return format;
				}
				else if (tiling == VK_IMAGE_TILING_OPTIMAL && (properties.optimalTilingFeatures & features) == features)
				{
					return format;
				}
			}

			// If no compatible format is found, return undefined.
			return VK_FORMAT_UNDEFINED;
		}

		VkExtent2D Renderer::ChooseSwapchainExtent(const VkSurfaceCapabilitiesKHR& capabilities) const
		{
			// If is not varying, just return the current extent of the window.
			if (capabilities.currentExtent.width != std::numeric_limits<u32>::max())
			{
				return capabilities.currentExtent;
			}
			else
			{
				VkExtent2D newExtent = {};
				newExtent.width = static_cast<u32>(_window->GetWidth());
				newExtent.height = static_cast<u32>(_window->GetHeight());

				// Clamp values by maximum extents of the surface capabilities.
				newExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, newExtent.width));
				newExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, newExtent.height));

				return newExtent;
			}
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

			CHECK_RESULT(vkCreateInstance(&createInfo, nullptr, &_instance), VK_SUCCESS, RendererResult::Failure)
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateLogicalDevice()
		{
			auto familyInfo = GetQueueFamilyInfo(_device._physical);
			boost::container::set<i32> queueFamilies = { familyInfo._graphicsFamily, familyInfo._presentationFamily, familyInfo._transferFamily };
			boost::container::vector<VkDeviceQueueCreateInfo> queueInfos(queueFamilies.size());

			// Create the information regarding the queues that will be used from the logical device.
			for (auto& family : queueFamilies)
			{
				float singlePriorities[] = { 1.0f };
				float doublePriorities[] = { 1.0f, 1.0f };

				// Configure queue info for specified family.
				queueInfos[family].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
				queueInfos[family].queueFamilyIndex = family;

				// Check if will require two queues from graphics family.
				if (family == familyInfo._graphicsFamily && !familyInfo.HasDedicatedTransfer())
				{
					queueInfos[family].queueCount = 2;
					queueInfos[family].pQueuePriorities = doublePriorities;
				}
				else
				{
					queueInfos[family].queueCount = 1;
					queueInfos[family].pQueuePriorities = singlePriorities;
				}
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
			
			CHECK_RESULT(vkCreateDevice(_device._physical, &createInfo, nullptr, &_device._logical), VK_SUCCESS, RendererResult::Failure)

			// Retrieve the queues created by the logical device and store handles.
			vkGetDeviceQueue(_device._logical, familyInfo._graphicsFamily, 0, &_graphicsQueue);
			vkGetDeviceQueue(_device._logical, familyInfo._presentationFamily, 0, &_presentationQueue);

			// Retrieve proper transfer queue created by the logical device.
			if (familyInfo.HasDedicatedTransfer())
			{
				vkGetDeviceQueue(_device._logical, familyInfo._transferFamily, 0, &_transferQueue);
			}
			else
			{
				vkGetDeviceQueue(_device._logical, familyInfo._transferFamily, 1, &_transferQueue);
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateSwapchain()
		{
			SwapchainInfo info = GetSwapchainInfo(_device._physical);

			// Find the optimal settings for the swapchain based on available information.
			VkSurfaceFormatKHR format = ChooseBestSurfaceFormat(info._formats);
			VkPresentModeKHR mode = ChooseBestPresentationMode(info._modes);
			VkExtent2D extent = ChooseSwapchainExtent(info._capabilities);

			// Pick the number of images in swapchain, attempting for triple buffering, but clamping on maximum count (0 is limitless).
			u32 imageCount = info._capabilities.maxImageCount > 0 ? std::min(info._capabilities.maxImageCount, info._capabilities.minImageCount + 1)
				: info._capabilities.minImageCount + 1;

			// Setup the information for the creation of the swapchain.
			VkSwapchainCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
			createInfo.surface = _surface;
			createInfo.imageFormat = format.format;
			createInfo.imageColorSpace = format.colorSpace;
			createInfo.presentMode = mode;
			createInfo.imageExtent = extent;
			createInfo.minImageCount = imageCount;
			createInfo.imageArrayLayers = 1;
			createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
			createInfo.preTransform = info._capabilities.currentTransform;
			createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
			createInfo.clipped = VK_TRUE;
			createInfo.oldSwapchain = VK_NULL_HANDLE;
			
			// If different families for graphics and presentation, need to share the swapchain between them.
			QueueFamilyInfo familyInfo = GetQueueFamilyInfo(_device._physical);
			if (familyInfo._graphicsFamily != familyInfo._presentationFamily)
			{
				u32 indices[] = { 
					(u32)familyInfo._graphicsFamily,
					(u32)familyInfo._presentationFamily
				};

				createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
				createInfo.queueFamilyIndexCount = 2;
				createInfo.pQueueFamilyIndices = indices;
			}
			else
			{
				createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
				createInfo.queueFamilyIndexCount = 0;
				createInfo.pQueueFamilyIndices = nullptr;
			}
			
			CHECK_RESULT(vkCreateSwapchainKHR(_device._logical, &createInfo, nullptr, &_swapchain), VK_SUCCESS, RendererResult::Failure)

			// Store format and extent for posterior usage.
			_swapchainFormat = format.format;
			_swapchainExtent = extent;

			// Get created swapchain images handles.
			u32 count = 0;
			vkGetSwapchainImagesKHR(_device._logical, _swapchain, &count, nullptr);

			boost::container::vector<VkImage> images(count);
			vkGetSwapchainImagesKHR(_device._logical, _swapchain, &count, images.data());

			for (auto img : images)
			{
				SwapchainImage swapchainImage = {};
				swapchainImage._raw = img;
				CHECK_RESULT(CreateImageView(img, _swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, &swapchainImage._view), RendererResult::Success, RendererResult::Failure)

				// Add to the list of swapchain images.
				_swapchainImages.emplace_back(swapchainImage);
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateRenderPass()
		{
			// Color attachment of the render pass.
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = _swapchainFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			// Depth attachment of the render pass.
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = _depthFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Attachment reference for color attachment of the subpass.
			VkAttachmentReference colorReference = {};
			colorReference.attachment = 0;
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Attachment reference for depth attachment of the subpass.
			VkAttachmentReference depthReference = {};
			depthReference.attachment = 1;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Subpass description for this render pass.
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorReference;
			subpass.pDepthStencilAttachment = &depthReference;

			// Need to determine when layout transition occurs through subpass dependencies.
			boost::container::vector<VkSubpassDependency> subpassDependencies(2);

			// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependencies[0].dstSubpass = 0;
			subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependencies[0].dependencyFlags = 0;

			// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			subpassDependencies[1].srcSubpass = 0;
			subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependencies[1].dependencyFlags = 0;

			// Assemble vector of attachments.
			boost::array<VkAttachmentDescription, 2> attachments = { 
				colorAttachment, 
				depthAttachment 
			};

			// Render pass creation information.
			VkRenderPassCreateInfo renderCreateInfo = {};
			renderCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderCreateInfo.attachmentCount = static_cast<u32>(attachments.size());
			renderCreateInfo.pAttachments = attachments.data();
			renderCreateInfo.subpassCount = 1;
			renderCreateInfo.pSubpasses = &subpass;
			renderCreateInfo.dependencyCount = static_cast<u32>(subpassDependencies.size());
			renderCreateInfo.pDependencies = subpassDependencies.data();
			
			CHECK_RESULT(vkCreateRenderPass(_device._logical, &renderCreateInfo, nullptr, &_renderPass), VK_SUCCESS, RendererResult::Failure)
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateDescriptorSetLayout()
		{
			// Descriptor set layout bindings information of the vertex uniform.
			VkDescriptorSetLayoutBinding vertexUniformLayoutBinding = {};
			vertexUniformLayoutBinding.binding = 0;
			vertexUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			vertexUniformLayoutBinding.descriptorCount = 1;
			vertexUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			vertexUniformLayoutBinding.pImmutableSamplers = nullptr;

			// Descriptor set layout bindings information of the fragment uniform.
			VkDescriptorSetLayoutBinding fragmentUniformLayoutBinding = {};
			fragmentUniformLayoutBinding.binding = 1;
			fragmentUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			fragmentUniformLayoutBinding.descriptorCount = 1;
			fragmentUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentUniformLayoutBinding.pImmutableSamplers = nullptr;

			// Descriptor set layout bindings information of the fragment dynamic uniform.
			VkDescriptorSetLayoutBinding fragmentDynamicUniformLayoutBinding = {};
			fragmentDynamicUniformLayoutBinding.binding = 2;
			fragmentDynamicUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			fragmentDynamicUniformLayoutBinding.descriptorCount = 1;
			fragmentDynamicUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentDynamicUniformLayoutBinding.pImmutableSamplers = nullptr;

			// Assemble the binding informations into an array.
			boost::array<VkDescriptorSetLayoutBinding, 3> layoutBindings = {
				vertexUniformLayoutBinding, fragmentUniformLayoutBinding, fragmentDynamicUniformLayoutBinding
			};

			// Descriptor set layout creation information.
			VkDescriptorSetLayoutCreateInfo layoutCreateInfo = {};
			layoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			layoutCreateInfo.bindingCount = static_cast<u32>(layoutBindings.size());
			layoutCreateInfo.pBindings = layoutBindings.data();

			// Create descriptor set layout with the logical device.
			CHECK_RESULT(vkCreateDescriptorSetLayout(_device._logical, &layoutCreateInfo, nullptr, &_descriptorSetLayout), VK_SUCCESS, RendererResult::Failure)
			return RendererResult::Success;
		}

		RendererResult Renderer::CreatePushConstantRanges()
		{
			// Resize the push constant range vector to hold all constants.
			_pushConstantRanges.resize(1);

			// Configure the values for the vertex push constant.
			_pushConstantRanges[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			_pushConstantRanges[0].offset = 0;
			_pushConstantRanges[0].size = sizeof(VertexPush);

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateGraphicsPipeline()
		{
			boost::container::vector<char> fragmentShaderRaw, vertexShaderRaw;
			CHECK_RESULT(ReadShader("Shaders/vert.spv", &vertexShaderRaw), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(ReadShader("Shaders/frag.spv", &fragmentShaderRaw), RendererResult::Success, RendererResult::Failure)

			// Create shader modules.
			VkShaderModule fragmentShader, vertexShader;
			CHECK_RESULT(CreateShaderModule(vertexShaderRaw, &vertexShader), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(CreateShaderModule(fragmentShaderRaw, &fragmentShader), RendererResult::Success, RendererResult::Failure)

			// Vertex stage creation information.
			VkPipelineShaderStageCreateInfo vertexCreateInfo = {};
			vertexCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			vertexCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			vertexCreateInfo.module = vertexShader;
			vertexCreateInfo.pName = "main";

			// Fragment stage creation information.
			VkPipelineShaderStageCreateInfo fragmentCreateInfo = {};
			fragmentCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			fragmentCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			fragmentCreateInfo.module = fragmentShader;
			fragmentCreateInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = {
				vertexCreateInfo,
				fragmentCreateInfo
			};

			/* Create graphics pipeline. */

			#pragma region Vertex Input Stage

			// Definitions of how data for a single vertex is a whole.
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			// How the data for an attribute is defined within the vertex.
			boost::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

			// Position attribute.
			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, _position);
			
			// Color attribute.
			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, _color);

			// Normal attribute.
			attributeDescriptions[2].binding = 0;
			attributeDescriptions[2].location = 2;
			attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[2].offset = offsetof(Vertex, _normal);

			// Vertex input stage.
			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
			vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
			vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription;
			vertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<u32>(attributeDescriptions.size());
			vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

			#pragma endregion
			#pragma region Input Assembly Stage

			// Input assembly stage.
			VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
			inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

			#pragma endregion
			#pragma region Viewport and Scissors Stage

			// Viewport and scissors stage.

			// Create the viewport info.
			VkViewport viewport = {};
			viewport.x = 0.0f;
			viewport.y = 0.0f;
			viewport.width = static_cast<float>(_swapchainExtent.width);
			viewport.height = static_cast<float>(_swapchainExtent.height);
			viewport.minDepth = 0.0f;
			viewport.maxDepth = 1.0f;

			// Create the scissor info. (specifies what is visible)
			VkRect2D scissor = {};
			scissor.offset = { 0, 0 };
			scissor.extent = _swapchainExtent;

			// Create the viewport and scissors state info.
			VkPipelineViewportStateCreateInfo viewportCreateInfo = {};
			viewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			viewportCreateInfo.viewportCount = 1;
			viewportCreateInfo.pViewports = &viewport;
			viewportCreateInfo.scissorCount = 1;
			viewportCreateInfo.pScissors = &scissor;

			#pragma endregion
			#pragma region Rasterization Stage

			// Rasterization stage.
			VkPipelineRasterizationStateCreateInfo rasterCreateInfo = {};
			rasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			rasterCreateInfo.depthClampEnable = VK_FALSE;
			rasterCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			rasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			rasterCreateInfo.lineWidth = 1.0f;
			rasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			rasterCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			rasterCreateInfo.depthBiasEnable = VK_FALSE;

			#pragma endregion
			#pragma region Multisampling Stage

			// Multisampling stage.
			VkPipelineMultisampleStateCreateInfo multisampleCreateInfo = {};
			multisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			multisampleCreateInfo.sampleShadingEnable = VK_FALSE;
			multisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			#pragma endregion
			#pragma region Blending Stage

			// Blending attachment information.
			VkPipelineColorBlendAttachmentState blendState = {};
			blendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | 
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			blendState.blendEnable = VK_TRUE;
			blendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			blendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			blendState.alphaBlendOp = VK_BLEND_OP_ADD;
			blendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			blendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			blendState.colorBlendOp = VK_BLEND_OP_ADD;

			// Using following equation for blending:
			// (VK_BLEND_FACTOR_SRC_ALPHA * newColor) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * oldColor)

			// Blending stage creation information.
			VkPipelineColorBlendStateCreateInfo blendCreateInfo = {};
			blendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			blendCreateInfo.logicOpEnable = VK_FALSE;
			blendCreateInfo.attachmentCount = 1;
			blendCreateInfo.pAttachments = &blendState;

			#pragma endregion
			#pragma region Pipeline Layout Stage

			// Pipeline layout stage creation information.
			VkPipelineLayoutCreateInfo layoutCreateInfo = {};
			layoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			layoutCreateInfo.setLayoutCount = 1;
			layoutCreateInfo.pSetLayouts = &_descriptorSetLayout;
			layoutCreateInfo.pushConstantRangeCount = static_cast<u32>(_pushConstantRanges.size());
			layoutCreateInfo.pPushConstantRanges = _pushConstantRanges.data();

			// Create pipeline layout.
			CHECK_RESULT(vkCreatePipelineLayout(_device._logical, &layoutCreateInfo, nullptr, &_pipelineLayout), VK_SUCCESS, RendererResult::Failure)
			
			#pragma endregion

			#pragma region Depth Stencil Stage

			// TODO: Setup depth stencil testing
			VkPipelineDepthStencilStateCreateInfo depthCreateInfo = {};
			depthCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			depthCreateInfo.depthTestEnable = VK_TRUE;
			depthCreateInfo.depthWriteEnable = VK_TRUE;
			depthCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
			depthCreateInfo.depthBoundsTestEnable = VK_FALSE;
			depthCreateInfo.stencilTestEnable = VK_FALSE;

			#pragma endregion

			// Create graphics pipeline information.
			VkGraphicsPipelineCreateInfo pipelineCreateInfo = {};
			pipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			pipelineCreateInfo.stageCount = 2;
			pipelineCreateInfo.pStages = shaderStages;
			pipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
			pipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
			pipelineCreateInfo.pViewportState = &viewportCreateInfo;
			pipelineCreateInfo.pDynamicState = nullptr;
			pipelineCreateInfo.pRasterizationState = &rasterCreateInfo;
			pipelineCreateInfo.pMultisampleState = &multisampleCreateInfo;
			pipelineCreateInfo.pColorBlendState = &blendCreateInfo;
			pipelineCreateInfo.pDepthStencilState = &depthCreateInfo;
			pipelineCreateInfo.layout = _pipelineLayout;
			pipelineCreateInfo.renderPass = _renderPass;							// Render pass it is compatible with.
			pipelineCreateInfo.subpass = 0;											// Subpass of the render pass that it will use.

			// Derivative graphics pipelines (use these attributes to base a pipeline onto another).
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineCreateInfo.basePipelineIndex = -1;

			CHECK_RESULT(vkCreateGraphicsPipelines(_device._logical, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_graphicsPipeline), VK_SUCCESS, RendererResult::Failure)
			
			// Destroy shader modules (no longer needed after creating pipeline).
			vkDestroyShaderModule(_device._logical, fragmentShader, nullptr);
			vkDestroyShaderModule(_device._logical, vertexShader, nullptr);

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateDepthBufferImage()
		{
			_depthFormat = ChooseBestSupportedFormat(
				{ VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL,
				VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
			);

			// Create and allocate image and view, using the appropriate depth stencil format.
			CHECK_RESULT(CreateImage(_swapchainExtent.width, _swapchainExtent.height, _depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, &_depthBufferImage), RendererResult::Success, RendererResult::Failure);
			CHECK_RESULT(AllocateImage(_depthBufferImage, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_depthBufferMemory), RendererResult::Success, RendererResult::Failure);
			CHECK_RESULT(CreateImageView(_depthBufferImage, _depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &_depthBufferImageView), RendererResult::Success, RendererResult::Failure);
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateFramebuffers()
		{
			_swapchainFramebuffers.resize(_swapchainImages.size());
			for (usize i = 0; i < _swapchainFramebuffers.size(); ++i)
			{
				boost::array<VkImageView, 2> attachments = {
					_swapchainImages[i]._view,
					_depthBufferImageView
				};

				VkFramebufferCreateInfo createInfo = {};
				createInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
				createInfo.renderPass = _renderPass;
				createInfo.attachmentCount = static_cast<u32>(attachments.size());
				createInfo.pAttachments = attachments.data();
				createInfo.width = _swapchainExtent.width;
				createInfo.height = _swapchainExtent.height;
				createInfo.layers = 1;

				CHECK_RESULT(vkCreateFramebuffer(_device._logical, &createInfo, nullptr, &_swapchainFramebuffers[i]), VK_SUCCESS, RendererResult::Failure)
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateCommandPools()
		{
			QueueFamilyInfo familyInfo = GetQueueFamilyInfo(_device._physical);

			// Create command pool for graphical operations.
			VkCommandPoolCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			createInfo.queueFamilyIndex = familyInfo._graphicsFamily;
			createInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

			for (usize i = 0; i < COMMAND_BUFFER_SETS; ++i)
			{
				CHECK_RESULT(vkCreateCommandPool(_device._logical, &createInfo, nullptr, &_graphicsPools[i]), VK_SUCCESS, RendererResult::Failure)
			}

			// Create command pool for transfer operations, if required.
			if (familyInfo.HasDedicatedTransfer())
			{
				VkCommandPoolCreateInfo transferCreateInfo = {};
				transferCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
				transferCreateInfo.queueFamilyIndex = familyInfo._transferFamily;
				transferCreateInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

				CHECK_RESULT(vkCreateCommandPool(_device._logical, &transferCreateInfo, nullptr, &_transferPool), VK_SUCCESS, RendererResult::Failure)
			}
			else
			{
				// Update flags for transfer operations.
				createInfo.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

				CHECK_RESULT(vkCreateCommandPool(_device._logical, &createInfo, nullptr, &_transferPool), VK_SUCCESS, RendererResult::Failure);
				//_transferPool = _graphicsPools[0];
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateCommandBuffers()
		{
			VkCommandBufferAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			// Create command buffers for graphical operations.
			for (usize i = 0; i < COMMAND_BUFFER_SETS; ++i)
			{
				_commandBuffers[i].resize(_swapchainFramebuffers.size());

				// Configure allocation for specific command buffer set.
				allocateInfo.commandPool = _graphicsPools[i];
				allocateInfo.commandBufferCount = static_cast<u32>(_commandBuffers[i].size());

				CHECK_RESULT(vkAllocateCommandBuffers(_device._logical, &allocateInfo, _commandBuffers[i].data()), VK_SUCCESS, RendererResult::Failure)
			}

			// Create command buffer for transfer operations.
			allocateInfo.commandPool = _transferPool;
			allocateInfo.commandBufferCount = 1;
			CHECK_RESULT(vkAllocateCommandBuffers(_device._logical, &allocateInfo, &_transferBuffer), VK_SUCCESS, RendererResult::Failure)

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateUniformBuffers()
		{
			VkDeviceSize vertexBufferSize = sizeof(VertexUniform);
			VkDeviceSize fragmentBufferSize = sizeof(FragmentUniform);
			VkDeviceSize fragmentDynamicBufferSize = GetUniformAlignment(sizeof(FragmentDynamicUniform)) * MAX_ENTITIES;

			for (usize i = 0; i < COMMAND_BUFFER_SETS; ++i)
			{
				// Create one vertex uniform buffer for each swapchain image.
				_vertexUniformBuffers[i].resize(_swapchainImages.size());
				_vertexUniformBuffersMemory[i].resize(_swapchainImages.size());
				for (usize j = 0; j < _vertexUniformBuffers[i].size(); ++j)
				{
					CHECK_RESULT(CreateBuffer(vertexBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &_vertexUniformBuffers[i][j]), RendererResult::Success, RendererResult::Failure)
					CHECK_RESULT(AllocateBuffer(_vertexUniformBuffers[i][j], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &_vertexUniformBuffersMemory[i][j]), RendererResult::Success, RendererResult::Failure)
				}

				// Create one fragment uniform buffer for each swapchain image.
				_fragmentUniformBuffers[i].resize(_swapchainImages.size());
				_fragmentUniformBuffersMemory[i].resize(_swapchainImages.size());
				for (usize j = 0; j < _fragmentUniformBuffers[i].size(); ++j)
				{
					CHECK_RESULT(CreateBuffer(fragmentBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &_fragmentUniformBuffers[i][j]), RendererResult::Success, RendererResult::Failure)
					CHECK_RESULT(AllocateBuffer(_fragmentUniformBuffers[i][j], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &_fragmentUniformBuffersMemory[i][j]), RendererResult::Success, RendererResult::Failure)
				}

				// Create one fragment dynamic uniform buffer for each swapchain image.
				_fragmentDynamicUniformBuffers[i].resize(_swapchainImages.size());
				_fragmentDynamicUniformBuffersMemory[i].resize(_swapchainImages.size());
				for (usize j = 0; j < _fragmentDynamicUniformBuffers[i].size(); ++j)
				{
					CHECK_RESULT(CreateBuffer(fragmentDynamicBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &_fragmentDynamicUniformBuffers[i][j]), RendererResult::Success, RendererResult::Failure)
					CHECK_RESULT(AllocateBuffer(_fragmentDynamicUniformBuffers[i][j], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &_fragmentDynamicUniformBuffersMemory[i][j]), RendererResult::Success, RendererResult::Failure)
				}
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateDescriptorPool()
		{
			// Information about pool for the vertex uniform descriptors.
			VkDescriptorPoolSize vertexPoolSize = {};
			vertexPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			vertexPoolSize.descriptorCount = static_cast<u32>(_swapchainImages.size() * COMMAND_BUFFER_SETS);

			// Information about pool for the fragment uniform descriptors.
			VkDescriptorPoolSize fragmentPoolSize = {};
			fragmentPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			fragmentPoolSize.descriptorCount = static_cast<u32>(_swapchainImages.size() * COMMAND_BUFFER_SETS);

			// Information about pool for the fragment dynamic uniform descriptors.
			VkDescriptorPoolSize fragmentDynamicPoolSize = {};
			fragmentDynamicPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			fragmentDynamicPoolSize.descriptorCount = static_cast<u32>(_swapchainImages.size() * COMMAND_BUFFER_SETS);

			// Assemble pool sizes into an array.
			boost::array<VkDescriptorPoolSize, 3> poolSizes = {
				vertexPoolSize, fragmentPoolSize, fragmentDynamicPoolSize
			};

			// Descriptor pool creation information.
			VkDescriptorPoolCreateInfo poolCreateInfo = {};
			poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			poolCreateInfo.maxSets = static_cast<u32>(_swapchainImages.size() * COMMAND_BUFFER_SETS);
			poolCreateInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
			poolCreateInfo.pPoolSizes = poolSizes.data();

			CHECK_RESULT(vkCreateDescriptorPool(_device._logical, &poolCreateInfo, nullptr, &_descriptorPool), VK_SUCCESS, RendererResult::Failure)
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateDescriptorSets()
		{
			for (usize i = 0; i < COMMAND_BUFFER_SETS; ++i)
			{
				// Setup a vector with layouts for each descriptor set created.
				boost::container::vector<VkDescriptorSetLayout> setLayouts(_swapchainImages.size(), _descriptorSetLayout);

				// Resize descriptor sets to match the uniform buffers they describe.
				_descriptorSets[i].resize(_swapchainImages.size());

				// Descriptor set allocation information.
				VkDescriptorSetAllocateInfo allocateInfo = {};
				allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
				allocateInfo.descriptorPool = _descriptorPool;
				allocateInfo.descriptorSetCount = static_cast<u32>(_swapchainImages.size());
				allocateInfo.pSetLayouts = setLayouts.data();

				// Allocate descriptor sets.
				CHECK_RESULT(vkAllocateDescriptorSets(_device._logical, &allocateInfo, _descriptorSets[i].data()), VK_SUCCESS, RendererResult::Failure)

				// Create the vectors to store descriptor set update information.
				boost::container::vector<VkDescriptorBufferInfo> descriptorBuffer;
				boost::container::vector<VkWriteDescriptorSet> descriptorWrite;

				// Resize vectors to fit every single descriptor set.
				descriptorBuffer.resize(3 * _descriptorSets[i].size());
				descriptorWrite.resize(3 * _descriptorSets[i].size());

				// Fill the information of the bindings between descriptor sets and uniform buffers.
				for (usize j = 0; j < _descriptorSets[i].size(); ++j)
				{
					// Vertex descriptor buffer information.
					descriptorBuffer[j].buffer = _vertexUniformBuffers[i][j];
					descriptorBuffer[j].offset = 0;
					descriptorBuffer[j].range = sizeof(VertexUniform);

					// Vertex write descriptor set information.
					descriptorWrite[j].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite[j].dstSet = _descriptorSets[i][j];
					descriptorWrite[j].dstBinding = 0;
					descriptorWrite[j].dstArrayElement = 0;
					descriptorWrite[j].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrite[j].descriptorCount = 1;
					descriptorWrite[j].pBufferInfo = &descriptorBuffer[j];

					// Fragment descriptor buffer information.
					descriptorBuffer[j + _descriptorSets[i].size()].buffer = _fragmentUniformBuffers[i][j];
					descriptorBuffer[j + _descriptorSets[i].size()].offset = 0;
					descriptorBuffer[j + _descriptorSets[i].size()].range = sizeof(FragmentUniform);

					// Fragment write descriptor set information.
					descriptorWrite[j + _descriptorSets[i].size()].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite[j + _descriptorSets[i].size()].dstSet = _descriptorSets[i][j];
					descriptorWrite[j + _descriptorSets[i].size()].dstBinding = 1;
					descriptorWrite[j + _descriptorSets[i].size()].dstArrayElement = 0;
					descriptorWrite[j + _descriptorSets[i].size()].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					descriptorWrite[j + _descriptorSets[i].size()].descriptorCount = 1;
					descriptorWrite[j + _descriptorSets[i].size()].pBufferInfo = &descriptorBuffer[j + _descriptorSets[i].size()];

					// Fragment dynamic descriptor buffer information.
					descriptorBuffer[j + 2 * _descriptorSets[i].size()].buffer = _fragmentDynamicUniformBuffers[i][j];
					descriptorBuffer[j + 2 * _descriptorSets[i].size()].offset = 0;
					descriptorBuffer[j + 2 * _descriptorSets[i].size()].range = GetUniformAlignment(sizeof(FragmentDynamicUniform));

					// Fragment dynamic descriptor set information.
					descriptorWrite[j + 2 * _descriptorSets[i].size()].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
					descriptorWrite[j + 2 * _descriptorSets[i].size()].dstSet = _descriptorSets[i][j];
					descriptorWrite[j + 2 * _descriptorSets[i].size()].dstBinding = 2;
					descriptorWrite[j + 2 * _descriptorSets[i].size()].dstArrayElement = 0;
					descriptorWrite[j + 2 * _descriptorSets[i].size()].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
					descriptorWrite[j + 2 * _descriptorSets[i].size()].descriptorCount = 1;
					descriptorWrite[j + 2 * _descriptorSets[i].size()].pBufferInfo = &descriptorBuffer[j + 2 * _descriptorSets[i].size()];
				}

				// Update the descriptor sets with the filled information.
				vkUpdateDescriptorSets(_device._logical, descriptorWrite.size(), descriptorWrite.data(), 0, nullptr);
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateSynchronization()
		{
			_imageAvailable.resize(MAX_FRAME_DRAWS);
			_renderFinished.resize(MAX_FRAME_DRAWS);
			_drawFences.resize(MAX_FRAME_DRAWS);

			// Semaphore creation information.
			VkSemaphoreCreateInfo semaphoreCreateInfo = {};
			semaphoreCreateInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

			// Fence creation information.
			VkFenceCreateInfo fenceCreateInfo = {};
			fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
			fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

			for (usize i = 0; i < MAX_FRAME_DRAWS; ++i)
			{
				CHECK_RESULT(vkCreateSemaphore(_device._logical, &semaphoreCreateInfo, nullptr, &_imageAvailable[i]), VK_SUCCESS, RendererResult::Failure)
				CHECK_RESULT(vkCreateSemaphore(_device._logical, &semaphoreCreateInfo, nullptr, &_renderFinished[i]), VK_SUCCESS, RendererResult::Failure)
				CHECK_RESULT(vkCreateFence(_device._logical, &fenceCreateInfo, nullptr, &_drawFences[i]), VK_SUCCESS, RendererResult::Failure)
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::RecordCommands(usize buffer, usize offset, usize size, bool isTransfer)
		{
			ASSERT(offset >= 0);
			ASSERT(size <= _commandBuffers[buffer].size());

			// Information about how to begin a command buffer.
			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			// Define the values to clear the framebuffer attachments with.
			boost::array<VkClearValue, 2> clearValues = {};
			clearValues[0].color = Math::Colors::Black;
			clearValues[1].depthStencil.depth = 1.0f;

			// Information about to begin a render pass.
			VkRenderPassBeginInfo passBeginInfo = {};
			passBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			passBeginInfo.renderPass = _renderPass;
			passBeginInfo.renderArea.offset = { 0, 0 };
			passBeginInfo.renderArea.extent = _swapchainExtent;
			passBeginInfo.clearValueCount = static_cast<u32>(clearValues.size());
			passBeginInfo.pClearValues = clearValues.data();
			
			for (usize i = offset; i < (offset + size); ++i)
			{
				// Set the correct framebuffer for the render pass.
				passBeginInfo.framebuffer = _swapchainFramebuffers[i];
				
				// Reset the command buffer.
				CHECK_RESULT(vkResetCommandBuffer(_commandBuffers[buffer][i], 0), VK_SUCCESS, RendererResult::Failure)
				CHECK_RESULT(vkBeginCommandBuffer(_commandBuffers[buffer][i], &bufferBeginInfo), VK_SUCCESS, RendererResult::Failure)
				vkCmdBeginRenderPass(_commandBuffers[buffer][i], &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				
				if (_entitiesToRender.size() > 0)
				{
					vkCmdBindPipeline(_commandBuffers[buffer][i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

					// Draw each renderable entity that has been streamed over to the renderer.
					if (isTransfer)
					{
						usize j = 0;
						BOOST_FOREACH(const auto& entity, boost::join(_entitiesToRender, _entitiesToTransfer))
						{
							RecordDraw(buffer, i, j, entity.second);
							j++;
						}
					}
					else
					{
						usize j = 0;
						BOOST_FOREACH(const auto& entity, _entitiesToRender)
						{
							RecordDraw(buffer, i, j, entity.second);
							j++;
						}
					}
				}

				vkCmdEndRenderPass(_commandBuffers[buffer][i]);
				CHECK_RESULT(vkEndCommandBuffer(_commandBuffers[buffer][i]), VK_SUCCESS, RendererResult::Failure)
			}
			
			return RendererResult::Success;
		}

		void Renderer::RecordDraw(usize bufferSet, usize bufferIndex, usize entityIndex, const RenderInfo& renderInfo)
		{
			// Acquire required information from the entities to be rendered.
			VkBuffer vertexBuffer = renderInfo._vertexBuffer;
			VkBuffer indexBuffer = renderInfo._indexBuffer;
			VkDeviceSize offsets[] = { 0 };
			u32 count = renderInfo._indexCount;

			// Bind vertex and index buffers.
			vkCmdBindVertexBuffers(_commandBuffers[bufferSet][bufferIndex], 0, 1, &vertexBuffer, offsets);
			vkCmdBindIndexBuffer(_commandBuffers[bufferSet][bufferIndex], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

			// Create vertex push constant from object and camera information.
			VertexPush vp = {};
			vp._model = renderInfo._transformComponent ? renderInfo._transformComponent->GetModel() : Math::Matrix::Identity();
			vp._view = _activeCamera ? _activeCamera->GetView() : Math::Matrix::Identity();

			// Push constants into the shaders.
			vkCmdPushConstants(_commandBuffers[bufferSet][bufferIndex], _pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 
				0, sizeof(VertexPush), &vp);

			// Configure dynamic offsets for dynamic buffers.
			u32 dynamicOffset = static_cast<u32>(GetUniformAlignment(sizeof(FragmentDynamicUniform))) * entityIndex;

			// Bind descriptor sets.
			vkCmdBindDescriptorSets(_commandBuffers[bufferSet][bufferIndex], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout,
				0, 1, &_descriptorSets[bufferSet][bufferIndex], 1, &dynamicOffset);

			// Add rendering commands to the buffer.
			vkCmdDrawIndexed(_commandBuffers[bufferSet][bufferIndex], count, 1, 0, 0, 0);
		}

		void Renderer::UpdatePointLight(FragmentUniform::FragmentPointLight* dstLight, const boost::shared_ptr<Entities::PointLight>& srcLight)
		{
			dstLight->_base._color = srcLight->GetColor();
			dstLight->_base._ambientStrength = srcLight->GetAmbientStrength();
			dstLight->_base._diffuseStrength = srcLight->GetDiffuseStrength();
			dstLight->_position = srcLight->GetPosition();
			dstLight->_constantAttenuation = srcLight->GetConstantAttenuation();
			dstLight->_linearAttenuation = srcLight->GetLinearAttenuation();
			dstLight->_quadraticAttenuation = srcLight->GetQuadraticAttenuation();
		}

		void Renderer::UpdateSpotLight(FragmentUniform::FragmentSpotLight* dstLight, const boost::shared_ptr<Entities::SpotLight>& srcLight)
		{
			UpdatePointLight(&dstLight->_base, srcLight);
			dstLight->_direction = srcLight->GetDirection();
			dstLight->_cutoffAngle = cosf(Math::ToRadians(srcLight->GetCutoffAngle()));
		}

		void Renderer::UpdateVertexUniformBuffers(const usize bufferSet)
		{
			for (usize i = 0; i < _vertexUniformBuffersMemory[bufferSet].size(); ++i)
			{
				// Map pointer to buffer location.
				void* data;
				vkMapMemory(_device._logical, _vertexUniformBuffersMemory[bufferSet][i], 0, sizeof(VertexUniform), 0, &data);

				// Transfer data to buffer memory.
				Memory::NMemCpy(data, &_vertexUniform, sizeof(VertexUniform));

				// Unmap memory from pointer.
				vkUnmapMemory(_device._logical, _vertexUniformBuffersMemory[bufferSet][i]);
			}
		}

		void Renderer::UpdateFragmentUniformBuffers(const usize bufferSet)
		{
			for (usize i = 0; i < _fragmentUniformBuffersMemory[bufferSet].size(); ++i)
			{
				// Map pointer to buffer location.
				void* data;
				vkMapMemory(_device._logical, _fragmentUniformBuffersMemory[bufferSet][i], 0, sizeof(FragmentUniform), 0, &data);

				// Transfer data to buffer memory.
				Memory::NMemCpy(data, &_fragmentUniform, sizeof(FragmentUniform));

				// Unmap memory from pointer.
				vkUnmapMemory(_device._logical, _fragmentUniformBuffersMemory[bufferSet][i]);
			}
		}

		void Renderer::UpdateFragmentDynamicUniformBuffers(const usize bufferSet)
		{
			const usize uniformAlignment = GetUniformAlignment(sizeof(FragmentDynamicUniform));
			for (usize i = 0; i < _fragmentDynamicUniformBuffersMemory[bufferSet].size(); ++i)
			{
				// Map pointer to buffer location.
				void* data;
				vkMapMemory(_device._logical, _fragmentDynamicUniformBuffersMemory[bufferSet][i], 0, uniformAlignment *
					_entitiesToRender.size() + _entitiesToTransfer.size(), 0, &data);

				// Copy entity data over to fragment dynamic buffer.
				usize j = 0;
				BOOST_FOREACH(const auto& entity, boost::join(_entitiesToRender, _entitiesToTransfer))
				{
					// Construct data to place into shader.
					FragmentDynamicUniform::FragmentMaterial material = {};
					material._specularPower = entity.second._material->GetSpecularPower();
					material._specularStrength = entity.second._material->GetSpecularStrength();

					FragmentDynamicUniform dynamicUniform = {};
					dynamicUniform._material = material;

					// Read memory with specified alignment.
					FragmentDynamicUniform* dynamicMemory = (FragmentDynamicUniform*)((usize)data + (j * uniformAlignment));
					*dynamicMemory = dynamicUniform;

					// Update index.
					j++;
				}

				// Unmap memory from pointer.
				vkUnmapMemory(_device._logical, _fragmentDynamicUniformBuffersMemory[bufferSet][i]);
			}
		}
		
		void Renderer::DestroyCommandPools()
		{
			for (usize i = 0; i < COMMAND_BUFFER_SETS; ++i)
			{
				vkDestroyCommandPool(_device._logical, _graphicsPools[i], nullptr);
			}

			vkDestroyCommandPool(_device._logical, _transferPool, nullptr);
		}

		void Renderer::DestroyEntities()
		{
			// Destroy each of the existing rendarable entities.
			for (auto& ent : _entitiesToRender)
			{
				DestroyBuffer(ent.second._vertexBuffer);
				DestroyBuffer(ent.second._indexBuffer);
			}
			
			_entitiesToRender.clear();
		}

		void Renderer::DestroySwapchain()
		{
			// Destroy each of the created swapchain views.
			for (auto& img : _swapchainImages)
			{
				vkDestroyImageView(_device._logical, img._view, nullptr);
			}

			vkDestroySwapchainKHR(_device._logical, _swapchain, nullptr);
		}

		void Renderer::DestroyDepthBufferImage()
		{
			vkDestroyImageView(_device._logical, _depthBufferImageView, nullptr);
			vkDestroyImage(_device._logical, _depthBufferImage, nullptr);
			vkFreeMemory(_device._logical, _depthBufferMemory, nullptr);
		}

		void Renderer::DestroyFramebuffers()
		{
			// Destroy each of the created framebuffers.
			for (auto& frame : _swapchainFramebuffers)
			{
				vkDestroyFramebuffer(_device._logical, frame, nullptr);
			}
		}

		void Renderer::DestroySynchronization()
		{
			for (usize i = 0; i < MAX_FRAME_DRAWS; ++i)
			{
				vkDestroySemaphore(_device._logical, _renderFinished[i], nullptr);
				vkDestroySemaphore(_device._logical, _imageAvailable[i], nullptr);
				vkDestroyFence(_device._logical, _drawFences[i], nullptr);
			}
		}

		void Renderer::DestroyUniformBuffers()
		{
			for (usize i = 0; i < COMMAND_BUFFER_SETS; ++i)
			{
				// Destroy vertex uniform buffers.
				for (usize j = 0; j < _vertexUniformBuffers[i].size(); ++j)
				{
					vkDestroyBuffer(_device._logical, _vertexUniformBuffers[i][j], nullptr);
					vkFreeMemory(_device._logical, _vertexUniformBuffersMemory[i][j], nullptr);
				}

				// Destroy fragment uniform buffers.
				for (usize j = 0; j < _fragmentUniformBuffers[i].size(); ++j)
				{
					vkDestroyBuffer(_device._logical, _fragmentUniformBuffers[i][j], nullptr);
					vkFreeMemory(_device._logical, _fragmentUniformBuffersMemory[i][j], nullptr);
				}

				// Destroy fragment dynamic uniform buffers.
				for (usize j = 0; j < _fragmentDynamicUniformBuffers[i].size(); ++j)
				{
					vkDestroyBuffer(_device._logical, _fragmentDynamicUniformBuffers[i][j], nullptr);
					vkFreeMemory(_device._logical, _fragmentDynamicUniformBuffersMemory[i][j], nullptr);
				}
			}
		}

		#if PLATFORM_WINDOWS
		RendererResult Renderer::CreateWindowsSurface(const Platform::Win32Window& window)
		{
			VkWin32SurfaceCreateInfoKHR createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
			createInfo.hinstance = window.GetInstance();
			createInfo.hwnd = window.GetHandle();

			CHECK_RESULT(vkCreateWin32SurfaceKHR(_instance, &createInfo, nullptr, &_surface), VK_SUCCESS, RendererResult::Failure)
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
			CHECK_RESULT(createFunction(_instance, &createInfo, nullptr, &_debugMessenger), VK_SUCCESS, RendererResult::Failure)
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

#endif // PLATFORM_USE_VULKAN
