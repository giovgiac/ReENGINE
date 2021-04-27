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
			: _currentFrame(0), _streamingQueue(512), _streamingThreadShouldClose(false), _releasedImages(512)
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

			// Re-write the command buffers to update values, if not already rerecording.
			CHECK_RESULT(RecordCommands(imageIndex, 1), RendererResult::Success, RendererResult::Failure);

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
			submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];
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
			CHECK_RESULT_WITH_ERROR(CreateGBuffer(), RendererResult::Success, NTEXT("Failed to create G-buffer!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateRenderPass(), RendererResult::Success, NTEXT("Failed to create renderpass!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateDescriptorSetLayouts(), RendererResult::Success, NTEXT("Failed to create descriptor set layouts!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreatePushConstantRanges(), RendererResult::Success, NTEXT("Failed to create push constant ranges!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateGraphicsPipelines(), RendererResult::Success, NTEXT("Failed to create graphics pipelines!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateFramebuffers(), RendererResult::Success, NTEXT("Failed to create framebuffers!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateCommandPools(), RendererResult::Success, NTEXT("Failed to create command pools!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateCommandBuffers(), RendererResult::Success, NTEXT("Failed to create command buffers!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateTextureSampler(), RendererResult::Success, NTEXT("Failed to create texture sampler!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateUniformBuffers(), RendererResult::Success, NTEXT("Failed to create uniform buffers!\n"), RendererResult::Failure)
			CHECK_RESULT_WITH_ERROR(CreateDescriptorPools(), RendererResult::Success, NTEXT("Failed to create descriptor pools!\n"), RendererResult::Failure)
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
			DestroyDescriptorPools();
			DestroyDescriptorSetLayouts();
			DestroyUniformBuffers();
			vkDestroySampler(_device._logical, _textureSampler, nullptr);
			DestroyCommandPools();
			DestroyFramebuffers();
			DestroyGraphicsPipelines();
			vkDestroyRenderPass(_device._logical, _renderPass, nullptr);
			DestroyGBuffer();
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
			// Update the fragment uniform with the directional light configuration.
			UpdateDirectionalLight(&_fragmentUniform._directionalLight, light);

			// Update the fragment uniform buffer values.
			UpdateLightingUniformBuffers();

			// Store specified light as active light.
			_directionalLight = light;
			_directionalLight->OnParameterChanged.connect([this]() {
				UpdateDirectionalLight(&_fragmentUniform._directionalLight, _directionalLight);
				UpdateLightingUniformBuffers();
			});

			return RendererResult::Success;
		}

		RendererResult Renderer::ActivateLight(const boost::shared_ptr<Entities::PointLight>& light)
		{
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
			UpdateLightingUniformBuffers();

			// Add new light to the list of point lights.
			_pointLights[availableIndex] = light;
			_pointLights[availableIndex]->OnParameterChanged.connect([this, availableIndex]() {
				UpdatePointLight(&_fragmentUniform._pointLights[availableIndex], _pointLights[availableIndex]);
				UpdateLightingUniformBuffers();
			});

			return RendererResult::Success;
		}

		RendererResult Renderer::ActivateLight(const boost::shared_ptr<Entities::SpotLight>& light)
		{
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
			UpdateLightingUniformBuffers();

			// Add new light to the list of spot lights.
			_spotLights[availableIndex] = light;
			_spotLights[availableIndex]->OnParameterChanged.connect([this, availableIndex]() {
				UpdateSpotLight(&_fragmentUniform._spotLights[availableIndex], _spotLights[availableIndex]);
				UpdateLightingUniformBuffers();
			});

			return RendererResult::Success;
		}

		void Renderer::DeactivateLight(const boost::shared_ptr<Entities::PointLight>& light)
		{
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
			UpdateLightingUniformBuffers();
		}

		void Renderer::DeactivateLight(const boost::shared_ptr<Entities::SpotLight>& light)
		{
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
			UpdateLightingUniformBuffers();
		}

		void Renderer::SetActiveCamera(const boost::shared_ptr<Entities::Camera>& newCamera)
		{
			// Retrieve projection matrix from the selected camera.
			_vertexUniform._projection = newCamera->GetProjection(static_cast<f32>(_swapchainExtent.width) / static_cast<f32>(_swapchainExtent.height));
			
			// Update the uniform buffer values.
			UpdateCameraUniformBuffers();

			// Store specified camera as active camera.
			_activeCamera = newCamera;
		}

		void Renderer::EntityStreaming()
		{
			static usize it = 0;
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
								// Create entity information to describe renderable entity.
								EntityInfo entityInfo = {};

								// Retrieve every render component belonging to the entity.
								auto renderComponents = transferInfo._entity->GetComponents<Components::RenderComponent>();

								// Resize renderables vector and record information for each one.
								usize i = 0;
								entityInfo._renderables.resize(renderComponents.size());
								for (auto& renderComponent : renderComponents)
								{
									auto indices = renderComponent->GetIndices();
									auto vertices = renderComponent->GetVertices();
									if (indices.size() == 0 || vertices.size() == 0) continue;

									// Create rendering information for renderable entity.
									RenderableInfo renderableInfo = {};
									renderableInfo._indexCount = static_cast<i32>(indices.size());
									renderableInfo._vertexCount = static_cast<i32>(vertices.size());
									renderableInfo._material = renderComponent->GetMaterial();

									// Create required buffers and images for the rendering.
									CreateIndexBuffer(indices, &renderableInfo._indexBuffer);
									CreateVertexBuffer(vertices, &renderableInfo._vertexBuffer);
									CreateTextureImage(renderableInfo._material->GetDiffuseTexture(), &renderableInfo._diffuseImage);

									// Assign renderable information to the entity.
									entityInfo._renderables[i++] = renderableInfo;
								}

								// Record the model matrix of the entity.
								if (transferInfo._entity->HasComponent<Components::TransformComponent>())
								{
									auto transformComponent = transferInfo._entity->GetComponent<Components::TransformComponent>();
									entityInfo._transformComponent = transformComponent;
								}

								// Place entity information to be transferred.
								_entitiesToTransfer.emplace_unique(transferInfo._entity, entityInfo);
							}
						}
						else
						{
							auto entity = _entitiesToRender.find(transferInfo._entity);
							if (entity != _entitiesToRender.end())
							{
								for (auto& renderableInfo : entity->second._renderables)
								{
									// Destroy buffers and images created for the rendering.
									DestroyBuffer(renderableInfo._indexBuffer);
									DestroyBuffer(renderableInfo._vertexBuffer);
									DestroyImage(renderableInfo._diffuseImage);
								}

								_entitiesToRender.erase(entity);
							}
						}
					}

					// Execute pending transfer operations.
					if (ExecuteTransferOperations() == RendererResult::Failure)
					{
						Core::Debug::Error("Failed to execute transfer operations.");
					}
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

		usize Renderer::GetAlignedSize(usize dataSize, usize dataAlignment) const
		{
			return (dataSize + dataAlignment - 1) & ~(dataAlignment - 1);
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

		RendererResult Renderer::CreateTextureImage(Texture* texture, VkImage* outImage)
		{
			if (!outImage) return RendererResult::Failure;

			if (_textureImage.left.find(texture) != _textureImage.left.end())
			{
				*outImage = _textureImage.left.at(texture);
				_textureReferences[*outImage] += 1;
				return RendererResult::Success;
			}
			else
			{
				// Load texture into memory.
				texture->Load();

				// Calculate image size.
				VkDeviceSize size = texture->GetWidth() * texture->GetHeight() * texture->GetBPP() * sizeof(u8);
				CHECK_RESULT(CreateImage(texture->GetWidth(), texture->GetHeight(), VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, outImage), RendererResult::Success, RendererResult::Failure)

				// Create texture information to be able to transfer.
				TextureInfo textureInfo = {};
				textureInfo._image = *outImage;
				textureInfo._size = 0;
				textureInfo._bpp = texture->GetBPP();
				textureInfo._width = texture->GetWidth();
				textureInfo._height = texture->GetHeight();
				textureInfo._pixels = texture->GetPixels();

				_textureImage.insert(boost::bimap<Texture*, VkImage>::value_type(texture, *outImage));
				_textureReferences.emplace_unique(*outImage, 1);
				_textureImagesToTransfer.emplace_back(textureInfo);
			}
			
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateTextureImageView(VkImage image)
		{
			if (_textureImageView.contains(image)) return RendererResult::Success;

			// Create and store the image view for the given image.
			VkImageView imageView;
			CHECK_RESULT(CreateImageView(image, VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &imageView), RendererResult::Success, RendererResult::Failure)
			
			// Save the image view in the map and return successfully.
			_textureImageView.emplace_unique(image, imageView);
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateTextureDescriptorSets()
		{
			// Verify images that require new descriptor sets.
			usize setCount = 0;
			for (const auto& tInfo : _textureImagesToTransfer)
			{
				if (!_textureDescriptorSets.contains(tInfo._image))
					setCount++;
			}

			if (setCount <= 0) return RendererResult::Success;

			boost::container::vector<VkDescriptorSet> descriptorSets;
			boost::container::vector<VkDescriptorSetLayout> setLayouts(setCount, _samplerDescriptorSetLayout);

			// Resize descriptor sets vector to fit.
			descriptorSets.resize(setCount);

			// Descriptor set allocation information.
			VkDescriptorSetAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			allocateInfo.descriptorPool = _samplerDescriptorPool;
			allocateInfo.descriptorSetCount = setCount;
			allocateInfo.pSetLayouts = setLayouts.data();

			CHECK_RESULT(vkAllocateDescriptorSets(_device._logical, &allocateInfo, descriptorSets.data()), VK_SUCCESS, RendererResult::Failure);

			boost::container::vector<VkDescriptorImageInfo> descriptorInfos;
			boost::container::vector<VkWriteDescriptorSet> descriptorWrites;

			// Resize structures to fit all images and descriptors.
			descriptorInfos.resize(setCount);
			descriptorWrites.resize(setCount);

			// Add descriptor sets to texture images, and generate their write structures.
			usize i = 0;
			for (const auto& tInfo : _textureImagesToTransfer)
			{
				if (_textureDescriptorSets.contains(tInfo._image)) continue;

				// Fill in the descriptor image information.
				descriptorInfos[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				descriptorInfos[i].imageView = _textureImageView[tInfo._image];
				descriptorInfos[i].sampler = _textureSampler;

				// Fill in the descriptor write structure.
				descriptorWrites[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[i].dstSet = descriptorSets[i];
				descriptorWrites[i].dstBinding = 0;
				descriptorWrites[i].dstArrayElement = 0;
				descriptorWrites[i].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				descriptorWrites[i].descriptorCount = 1;
				descriptorWrites[i].pImageInfo = &descriptorInfos[i];

				// Associate the descriptor set with the image in the map structure.
				_textureDescriptorSets.emplace_unique(tInfo._image, descriptorSets[i]);
				i++;
			}

			vkUpdateDescriptorSets(_device._logical, descriptorWrites.size(), descriptorWrites.data(), 0, nullptr);
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

		void Renderer::DestroyImage(VkImage image)
		{
			if (_imageMemory.left.find(image) != _imageMemory.left.end())
			{
				if (_textureReferences.contains(image) && _textureReferences[image] <= 1)
				{
					VkDeviceMemory memory = _imageMemory.left.at(image);
					usize distance = std::distance(_imageMemory.right.lower_bound(memory), _imageMemory.right.upper_bound(memory));

					// Free the descriptor set.
					if (_textureDescriptorSets.contains(image))
						vkFreeDescriptorSets(_device._logical, _samplerDescriptorPool, 1, &_textureDescriptorSets[image]);

					// Destroy the image view.
					if (_textureImageView.contains(image))
						vkDestroyImageView(_device._logical, _textureImageView[image], nullptr);

					// Destroy the image and remove it from references.
					vkDestroyImage(_device._logical, image, nullptr);
					_imageMemory.left.erase(image);
					_textureReferences.erase(image);

					// Free the image memory.
					if (distance <= 1)
					{
						vkFreeMemory(_device._logical, memory, nullptr);
						_imageMemory.right.erase(memory);
					}
				}
				else
				{
					if (_textureReferences.contains(image))
					{
						_textureReferences[image]--;
					}
				}
			}
			else
			{
				if (_textureReferences.contains(image) && _textureReferences[image] <= 1)
				{
					// Free the descriptor set.
					if (_textureDescriptorSets.contains(image))
						vkFreeDescriptorSets(_device._logical, _samplerDescriptorPool, 1, &_textureDescriptorSets[image]);

					// Destroy the image view.
					if (_textureImageView.contains(image))
						vkDestroyImageView(_device._logical, _textureImageView[image], nullptr);

					// Destroy the image and remove it from references.
					vkDestroyImage(_device._logical, image, nullptr);
					_imageMemory.left.erase(image);
					_textureReferences.erase(image);
				}
				else
				{
					if (_textureReferences.contains(image))
					{
						_textureReferences[image]--;
					}
				}
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
				vInfo._size = GetAlignedSize(memoryRequirements.size, memoryRequirements.alignment);

				// Update general values for big chunk of memory.
				vertexAllocationSize += vInfo._size;
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
				iInfo._size = GetAlignedSize(memoryRequirements.size, memoryRequirements.alignment);

				// Update general values for big chunk of memory.
				indexAllocationSize += iInfo._size;
				indexAllowedTypes |= memoryRequirements.memoryTypeBits;
			}

			// Calculate proper memory size and type for textures.
			usize imageAllocationSize = 0;
			u32 imageAllowedTypes = 0;
			for (auto& tInfo : _textureImagesToTransfer)
			{
				// Get image memory requirements.
				VkMemoryRequirements memoryRequirements;
				vkGetImageMemoryRequirements(_device._logical, tInfo._image, &memoryRequirements);

				// Update size of individual image.
				tInfo._size = GetAlignedSize(memoryRequirements.size, memoryRequirements.alignment);

				// Update general values for big chunk of memory.
				imageAllocationSize += tInfo._size;
				imageAllowedTypes |= memoryRequirements.memoryTypeBits;
			}

			// Create staging buffers, allocate memory and copy data over to them.
			// u32 i = sizeof(Vertex);
			VkBuffer vertexStagingBuffer = VK_NULL_HANDLE, indexStagingBuffer = VK_NULL_HANDLE, imageStagingBuffer = VK_NULL_HANDLE;
			VkDeviceMemory vertexStagingMemory = VK_NULL_HANDLE, indexStagingMemory = VK_NULL_HANDLE, imageStagingMemory = VK_NULL_HANDLE;
			CHECK_RESULT(StageVertexBuffer(vertexAllocationSize, &vertexStagingBuffer, &vertexStagingMemory), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(StageIndexBuffer(indexAllocationSize, &indexStagingBuffer, &indexStagingMemory), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(StageImageBuffer(imageAllocationSize, &imageStagingBuffer, &imageStagingMemory), RendererResult::Success, RendererResult::Failure)

			// Allocate memory to real vertex buffer.
			VkDeviceMemory vertexMemory = VK_NULL_HANDLE;
			if (vertexAllocationSize > 0)
			{
				VkMemoryAllocateInfo vertexAllocateInfo = {};
				vertexAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				vertexAllocateInfo.allocationSize = vertexAllocationSize;
				vertexAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(vertexAllowedTypes, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				CHECK_RESULT(vkAllocateMemory(_device._logical, &vertexAllocateInfo, nullptr, &vertexMemory), VK_SUCCESS, RendererResult::Failure)
			}

			// Allocate memory to real index buffer.
			VkDeviceMemory indexMemory = VK_NULL_HANDLE;
			if (indexAllocationSize > 0)
			{
				VkMemoryAllocateInfo indexAllocateInfo = {};
				indexAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				indexAllocateInfo.allocationSize = indexAllocationSize;
				indexAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(indexAllowedTypes, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				CHECK_RESULT(vkAllocateMemory(_device._logical, &indexAllocateInfo, nullptr, &indexMemory), VK_SUCCESS, RendererResult::Failure)
			}

			// Allocate memory to real image.
			VkDeviceMemory imageMemory = VK_NULL_HANDLE;
			if (imageAllocationSize > 0)
			{
				VkMemoryAllocateInfo imageAllocateInfo = {};
				imageAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
				imageAllocateInfo.allocationSize = imageAllocationSize;
				imageAllocateInfo.memoryTypeIndex = FindMemoryTypeIndex(imageAllowedTypes, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

				CHECK_RESULT(vkAllocateMemory(_device._logical, &imageAllocateInfo, nullptr, &imageMemory), VK_SUCCESS, RendererResult::Failure);
			}

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

			// Bind memory to real image.
			VkDeviceSize imageOffset = 0;
			for (const auto& tInfo : _textureImagesToTransfer)
			{
				CHECK_RESULT(vkBindImageMemory(_device._logical, tInfo._image, imageMemory, imageOffset), VK_SUCCESS, RendererResult::Failure)
				CHECK_RESULT(CreateTextureImageView(tInfo._image), RendererResult::Success, RendererResult::Failure);
				
				_imageMemory.insert(boost::bimap<VkImage, VkDeviceMemory>::value_type(tInfo._image, imageMemory));
				imageOffset += tInfo._size;
			}

			CreateTextureDescriptorSets();

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
				// Configure region of buffers to copy from.
				VkBufferCopy region = {};
				region.srcOffset = vertexSrcOffset;
				region.dstOffset = 0;
				region.size = vInfo._vertices.size() * sizeof(Vertex);

				// Add copy operation to the command buffer.
				vkCmdCopyBuffer(_transferBuffer, vertexStagingBuffer, vInfo._buffer, 1, &region);
				vertexSrcOffset += vInfo._size;
			}

			VkDeviceSize indexSrcOffset = 0;
			for (const auto& iInfo : _indexBuffersToTransfer)
			{
				// Configure region of buffers to copy from.
				VkBufferCopy region = {};
				region.srcOffset = indexSrcOffset;
				region.dstOffset = 0;
				region.size = iInfo._indices.size() * sizeof(u32);

				// Add copy operation to the command buffer.
				vkCmdCopyBuffer(_transferBuffer, indexStagingBuffer, iInfo._buffer, 1, &region);
				indexSrcOffset += iInfo._size;
			}

			VkDeviceSize imageSrcOffset = 0;
			for (const auto& tInfo : _textureImagesToTransfer)
			{
				// Configure region of buffers to copy from.
				VkBufferImageCopy region = {};
				region.bufferOffset = imageSrcOffset;
				region.bufferRowLength = 0;
				region.bufferImageHeight = 0;
				region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
				region.imageSubresource.mipLevel = 0;
				region.imageSubresource.baseArrayLayer = 0;
				region.imageSubresource.layerCount = 1;
				region.imageOffset = { 0, 0, 0 };
				region.imageExtent = { tInfo._width, tInfo._height, 1 };
				
				// Request to transition the image to receive data.
				TransitionImageLayout(_transferBuffer, tInfo._image, VK_QUEUE_FAMILY_IGNORED, VK_QUEUE_FAMILY_IGNORED, VK_IMAGE_LAYOUT_UNDEFINED,
					VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 0, VK_ACCESS_TRANSFER_WRITE_BIT, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
				
				// Add copy operation to the command buffer.
				vkCmdCopyBufferToImage(_transferBuffer, imageStagingBuffer, tInfo._image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
				
				// Configure memory barrier to transition the image to be read from the fragment shader.
				TransitionImageLayout(_transferBuffer, tInfo._image, _transferQueueFamily, _graphicsQueueFamily, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
					VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_ACCESS_TRANSFER_WRITE_BIT, 0, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

				imageSrcOffset += tInfo._size;
			}

			CHECK_RESULT(vkEndCommandBuffer(_transferBuffer), VK_SUCCESS, RendererResult::Failure)

			// Configure queue submission information.
			VkSubmitInfo submitInfo = {};
			submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
			submitInfo.commandBufferCount = 1;
			submitInfo.pCommandBuffers = &_transferBuffer;

			// Submit command to queue and wait until it finishes.
			CHECK_RESULT(vkQueueSubmit(_transferQueue, 1, &submitInfo, VK_NULL_HANDLE), VK_SUCCESS, RendererResult::Failure)

			// TODO: In the future, record secondary command buffers for static objects.
			
			UpdateMaterialUniformBuffers();

			// Wait until the transfer operations are completed before proceeding.
			CHECK_RESULT(vkQueueWaitIdle(_transferQueue), VK_SUCCESS, RendererResult::Failure);

			// Push texture images after their layout has been released.
			_transferMutex.lock();
			for (const auto& tInfo : _textureImagesToTransfer)
				_releasedImages.push(tInfo._image);

			// Add new entities to renderable entities.
			_entitiesToRender.insert(_entitiesToTransfer.begin(), _entitiesToTransfer.end());
			_transferMutex.unlock();

			// Cleanup stage buffers and free their memory.
			if (imageStagingBuffer && imageStagingMemory)
			{
				vkDestroyBuffer(_device._logical, imageStagingBuffer, nullptr);
				vkFreeMemory(_device._logical, imageStagingMemory, nullptr);
			}

			if (indexStagingBuffer && indexStagingMemory)
			{
				vkDestroyBuffer(_device._logical, indexStagingBuffer, nullptr);
				vkFreeMemory(_device._logical, indexStagingMemory, nullptr);
			}

			if (vertexStagingBuffer && vertexStagingMemory)
			{
				vkDestroyBuffer(_device._logical, vertexStagingBuffer, nullptr);
				vkFreeMemory(_device._logical, vertexStagingMemory, nullptr);
			}

			// Cleanup data structures containing buffers and entities to transfer.
			_vertexBuffersToTransfer.clear();
			_indexBuffersToTransfer.clear();
			_textureImagesToTransfer.clear();
			_entitiesToTransfer.clear();

			return RendererResult::Success;
		}

		RendererResult Renderer::StageIndexBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory)
		{
			if (bufferSize <= 0) return RendererResult::Success;

			// Create staging buffer.
			CHECK_RESULT(CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(AllocateBuffer(*stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemory), RendererResult::Success, RendererResult::Failure)

			// Map pointer to staging buffer location and transfer data.
			void* data;
			CHECK_RESULT(vkMapMemory(_device._logical, *stagingMemory, 0, bufferSize, 0, &data), VK_SUCCESS, RendererResult::Failure)

			VkDeviceSize stagingOffset = 0;
			for (const auto& iInfo : _indexBuffersToTransfer)
			{
				Memory::Copy((u8*)data + stagingOffset, iInfo._indices.data(), (usize)(iInfo._indices.size() * sizeof(u32)));
				stagingOffset += iInfo._size;
			}

			vkUnmapMemory(_device._logical, *stagingMemory);
			return RendererResult::Success;
		}

		RendererResult Renderer::StageVertexBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory)
		{
			if (bufferSize <= 0) return RendererResult::Success;

			// Create staging buffer.
			CHECK_RESULT(CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(AllocateBuffer(*stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemory), RendererResult::Success, RendererResult::Failure)

			// Map pointer to staging buffer location and transfer data.
			void* data;
			CHECK_RESULT(vkMapMemory(_device._logical, *stagingMemory, 0, bufferSize, 0, &data), VK_SUCCESS, RendererResult::Failure)

			VkDeviceSize stagingOffset = 0;
			for (const auto& vInfo : _vertexBuffersToTransfer)
			{
				Memory::Copy((u8*)data + stagingOffset, vInfo._vertices.data(), (usize)(vInfo._vertices.size() * sizeof(Vertex)));
				stagingOffset += vInfo._size;
			}

			vkUnmapMemory(_device._logical, *stagingMemory);
			return RendererResult::Success;
		}

		RendererResult Renderer::StageImageBuffer(VkDeviceSize bufferSize, VkBuffer* stagingBuffer, VkDeviceMemory* stagingMemory)
		{
			if (bufferSize <= 0) return RendererResult::Success;

			// Create staging buffer.
			CHECK_RESULT(CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingBuffer), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(AllocateBuffer(*stagingBuffer, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingMemory), RendererResult::Success, RendererResult::Failure)

			// Map pointer to staging buffer location and transfer data.
			void* data;
			CHECK_RESULT(vkMapMemory(_device._logical, *stagingMemory, 0, bufferSize, 0, &data), VK_SUCCESS, RendererResult::Failure)

			VkDeviceSize stagingOffset = 0;
			for (const auto& tInfo : _textureImagesToTransfer)
			{
				Memory::Copy((u8*)data + stagingOffset, tInfo._pixels, tInfo._width * tInfo._height * tInfo._bpp * sizeof(u8));
			 	stagingOffset += tInfo._size;

				// Release texture image from RAM.
				_textureImage.right.at(tInfo._image)->Unload();
			}

			vkUnmapMemory(_device._logical, *stagingMemory);

			return RendererResult::Success;
		}

		void Renderer::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImage image, u32 srcQueueFamily, u32 dstQueueFamily, VkImageLayout srcLayout, VkImageLayout dstLayout, VkAccessFlags srcAccess, VkAccessFlags dstAccess, VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage)
		{
			// Configure memory barrier to transition the image.
			VkImageMemoryBarrier memoryBarrier = {};
			memoryBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
			memoryBarrier.oldLayout = srcLayout;
			memoryBarrier.newLayout = dstLayout;
			memoryBarrier.srcQueueFamilyIndex = srcQueueFamily;
			memoryBarrier.dstQueueFamilyIndex = dstQueueFamily;
			memoryBarrier.image = image;
			memoryBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			memoryBarrier.subresourceRange.baseMipLevel = 0;
			memoryBarrier.subresourceRange.levelCount = 1;
			memoryBarrier.subresourceRange.baseArrayLayer = 0;
			memoryBarrier.subresourceRange.layerCount = 1;
			memoryBarrier.srcAccessMask = srcAccess;
			memoryBarrier.dstAccessMask = dstAccess;

			// Add a pipeline barrier to transition the texture layout.
			vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &memoryBarrier);
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

			// TODO: Verify the presence of features in CheckPhysicalDeviceSuitable
			// Construct the features from the device that we are going to use.
			VkPhysicalDeviceFeatures features = {};
			features.samplerAnisotropy = VK_TRUE;
			
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

			_graphicsQueueFamily = familyInfo._graphicsFamily;
			_transferQueueFamily = familyInfo._transferFamily;

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
			boost::array<VkSubpassDescription, 2> subpassDescriptions = {};

			// SUBPASS 1

			// Color attachment of the first subpass.
			VkAttachmentDescription colorAttachment = {};
			colorAttachment.format = _colorFormat;
			colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Depth attachment of the first subpass.
			VkAttachmentDescription depthAttachment = {};
			depthAttachment.format = _depthFormat;
			depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Normal attachment of the first subpass.
			VkAttachmentDescription normalAttachment = {};
			normalAttachment.format = _normalFormat;
			normalAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			normalAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			normalAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			normalAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			normalAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			normalAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			normalAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Position attachment of the first subpass.
			VkAttachmentDescription positionAttachment = {};
			positionAttachment.format = _positionFormat;
			positionAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			positionAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			positionAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			positionAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			positionAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			positionAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			positionAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Reference for color attachment of the first subpass.
			VkAttachmentReference colorReference = {};
			colorReference.attachment = 0;
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Reference for depth attachment of the first subpass.
			VkAttachmentReference depthReference = {};
			depthReference.attachment = 1;
			depthReference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

			// Reference for normal attachment of the first subpass.
			VkAttachmentReference normalReference = {};
			normalReference.attachment = 2;
			normalReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Reference for position attachment of the first subpass.
			VkAttachmentReference positionReference = {};
			positionReference.attachment = 3;
			positionReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Assemble color and normal attachments into an array.
			boost::array<VkAttachmentReference, 3> colorReferences = {
				colorReference, normalReference, positionReference
			};

			// Description for the first subpass.
			subpassDescriptions[0].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescriptions[0].colorAttachmentCount = static_cast<u32>(colorReferences.size());
			subpassDescriptions[0].pColorAttachments = colorReferences.data();
			subpassDescriptions[0].pDepthStencilAttachment = &depthReference;

			// SUBPASS 2

			// Output attachment of the second subpass.
			VkAttachmentDescription swapchainAttachment = {};
			swapchainAttachment.format = _swapchainFormat;
			swapchainAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
			swapchainAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
			swapchainAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			swapchainAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			swapchainAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			swapchainAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			swapchainAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

			// Attachment reference for color attachment of the subpass.
			VkAttachmentReference swapchainReference = {};
			swapchainReference.attachment = 4;
			swapchainReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Create attachment references to first subpass.
			boost::array<VkAttachmentReference, 4> inputReferences;
			for (usize i = 0; i < inputReferences.size(); ++i)
			{
				inputReferences[i].attachment = i;
				inputReferences[i].layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			}

			// Description for the second subpass.
			subpassDescriptions[1].pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpassDescriptions[1].colorAttachmentCount = 1;
			subpassDescriptions[1].pColorAttachments = &swapchainReference;
			subpassDescriptions[1].inputAttachmentCount = static_cast<u32>(inputReferences.size());
			subpassDescriptions[1].pInputAttachments = inputReferences.data();

			// Need to determine when layout transition occurs through subpass dependencies.
			boost::array<VkSubpassDependency, 3> subpassDependencies;

			// Conversion from VK_IMAGE_LAYOUT_UNDEFINED to VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
			subpassDependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependencies[0].dstSubpass = 0;
			subpassDependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependencies[0].dependencyFlags = 0;

			// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
			subpassDependencies[1].srcSubpass = 0;
			subpassDependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependencies[1].dstSubpass = 1;
			subpassDependencies[1].dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			subpassDependencies[1].dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
			subpassDependencies[1].dependencyFlags = 0;

			// Conversion from VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL to VK_IMAGE_LAYOUT_PRESENT_SRC_KHR
			subpassDependencies[2].srcSubpass = 1;
			subpassDependencies[2].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			subpassDependencies[2].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			subpassDependencies[2].dstSubpass = VK_SUBPASS_EXTERNAL;
			subpassDependencies[2].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
			subpassDependencies[2].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
			subpassDependencies[2].dependencyFlags = 0;

			// Assemble vector of attachments.
			boost::array<VkAttachmentDescription, 5> attachments = { 
				colorAttachment, depthAttachment, normalAttachment, positionAttachment, swapchainAttachment
			};

			// Render pass creation information.
			VkRenderPassCreateInfo renderCreateInfo = {};
			renderCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderCreateInfo.attachmentCount = static_cast<u32>(attachments.size());
			renderCreateInfo.pAttachments = attachments.data();
			renderCreateInfo.subpassCount = static_cast<u32>(subpassDescriptions.size());
			renderCreateInfo.pSubpasses = subpassDescriptions.data();
			renderCreateInfo.dependencyCount = static_cast<u32>(subpassDependencies.size());
			renderCreateInfo.pDependencies = subpassDependencies.data();
			
			CHECK_RESULT(vkCreateRenderPass(_device._logical, &renderCreateInfo, nullptr, &_renderPass), VK_SUCCESS, RendererResult::Failure)
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateDescriptorSetLayouts()
		{
			// BUFFER DESCRIPTOR SET LAYOUT (G-BUFFER PIPELINE)

			// Descriptor set layout bindings information of the vertex uniform.
			VkDescriptorSetLayoutBinding cameraUniformLayoutBinding = {};
			cameraUniformLayoutBinding.binding = 0;
			cameraUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			cameraUniformLayoutBinding.descriptorCount = 1;
			cameraUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			cameraUniformLayoutBinding.pImmutableSamplers = nullptr;

			// Descriptor set layout bindings information of the fragment dynamic uniform.
			VkDescriptorSetLayoutBinding materialUniformLayoutBinding = {};
			materialUniformLayoutBinding.binding = 1;
			materialUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			materialUniformLayoutBinding.descriptorCount = 1;
			materialUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			materialUniformLayoutBinding.pImmutableSamplers = nullptr;

			// Assemble the binding informations into an array.
			boost::array<VkDescriptorSetLayoutBinding, 2> bufferLayoutBindings = {
				cameraUniformLayoutBinding, materialUniformLayoutBinding
			};

			// Buffer descriptor set layout creation information.
			VkDescriptorSetLayoutCreateInfo bufferLayoutCreateInfo = {};
			bufferLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			bufferLayoutCreateInfo.bindingCount = static_cast<u32>(bufferLayoutBindings.size());
			bufferLayoutCreateInfo.pBindings = bufferLayoutBindings.data();

			CHECK_RESULT(vkCreateDescriptorSetLayout(_device._logical, &bufferLayoutCreateInfo, nullptr, &_bufferDescriptorSetLayout), VK_SUCCESS, RendererResult::Failure)
			
			// SAMPLER DESCRIPTOR SET LAYOUT (G-BUFFER PIPELINE)

			// Descriptor set layout bindings information of the texture sampler.
			VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
			samplerLayoutBinding.binding = 0;
			samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerLayoutBinding.descriptorCount = 1;
			samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			samplerLayoutBinding.pImmutableSamplers = nullptr;

			// Sampler descriptor set layout creation information.
			VkDescriptorSetLayoutCreateInfo samplerLayoutCreateInfo = {};
			samplerLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			samplerLayoutCreateInfo.bindingCount = 1;
			samplerLayoutCreateInfo.pBindings = &samplerLayoutBinding;

			CHECK_RESULT(vkCreateDescriptorSetLayout(_device._logical, &samplerLayoutCreateInfo, nullptr, &_samplerDescriptorSetLayout), VK_SUCCESS, RendererResult::Failure)

			// G-BUFFER DESCRIPTOR SET LAYOUT (LIGHTING PIPELINE)

			// Descriptor set layout binding of the color image.
			VkDescriptorSetLayoutBinding colorLayoutBinding = {};
			colorLayoutBinding.binding = 0;
			colorLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			colorLayoutBinding.descriptorCount = 1;
			colorLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			colorLayoutBinding.pImmutableSamplers = nullptr;

			// Descriptor set layout binding of the depth image.
			VkDescriptorSetLayoutBinding depthLayoutBinding = {};
			depthLayoutBinding.binding = 1;
			depthLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			depthLayoutBinding.descriptorCount = 1;
			depthLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			depthLayoutBinding.pImmutableSamplers = nullptr;

			// Descriptor set layout binding of the normal image.
			VkDescriptorSetLayoutBinding normalLayoutBinding = {};
			normalLayoutBinding.binding = 2;
			normalLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			normalLayoutBinding.descriptorCount = 1;
			normalLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			normalLayoutBinding.pImmutableSamplers = nullptr;

			// Descriptor set layout binding of the position image.
			VkDescriptorSetLayoutBinding positionLayoutBinding = {};
			positionLayoutBinding.binding = 3;
			positionLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			positionLayoutBinding.descriptorCount = 1;
			positionLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			positionLayoutBinding.pImmutableSamplers = nullptr;

			// Assemble descriptor set layout bindings into an array.
			boost::array<VkDescriptorSetLayoutBinding, 4> gBufferLayoutBindings = {
				colorLayoutBinding, depthLayoutBinding, normalLayoutBinding, positionLayoutBinding
			};

			// G-buffer descriptor set layout creation information.
			VkDescriptorSetLayoutCreateInfo gBufferLayoutCreateInfo = {};
			gBufferLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			gBufferLayoutCreateInfo.bindingCount = static_cast<u32>(gBufferLayoutBindings.size());
			gBufferLayoutCreateInfo.pBindings = gBufferLayoutBindings.data();

			CHECK_RESULT(vkCreateDescriptorSetLayout(_device._logical, &gBufferLayoutCreateInfo, nullptr, &_gBufferDescriptorSetLayout), VK_SUCCESS, RendererResult::Failure)

			// LIGHTING DESCRIPTOR SET LAYOUT (LIGHTING PIPELINE)

			// Descriptor set layout bindings information of the fragment uniform.
			VkDescriptorSetLayoutBinding lightingUniformLayoutBinding = {};
			lightingUniformLayoutBinding.binding = 0;
			lightingUniformLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lightingUniformLayoutBinding.descriptorCount = 1;
			lightingUniformLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			lightingUniformLayoutBinding.pImmutableSamplers = nullptr;

			// Lighting descriptor set layout creation information.
			VkDescriptorSetLayoutCreateInfo lightingLayoutCreateInfo = {};
			lightingLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			lightingLayoutCreateInfo.bindingCount = 1;
			lightingLayoutCreateInfo.pBindings = &lightingUniformLayoutBinding;

			CHECK_RESULT(vkCreateDescriptorSetLayout(_device._logical, &lightingLayoutCreateInfo, nullptr, &_lightingDescriptorSetLayout), VK_SUCCESS, RendererResult::Failure)

			return RendererResult::Success;
		}

		RendererResult Renderer::CreatePushConstantRanges()
		{
			// Configure the values for the G-buffer push constant.
			_gBufferPushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
			_gBufferPushConstantRange.offset = 0;
			_gBufferPushConstantRange.size = sizeof(GBufferPush);

			// Configure the values for the lighting push constant.
			_lightingPushConstantRange.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
			_lightingPushConstantRange.offset = 0;
			_lightingPushConstantRange.size = sizeof(LightingPush);

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateGraphicsPipelines()
		{
			// G-BUFFER GRAPHICS PIPELINE

			boost::container::vector<char> gBufferVertexShaderRaw, gBufferFragmentShaderRaw;
			CHECK_RESULT(ReadShader("Shaders/Deferred/gbuffer.vert.spv", &gBufferVertexShaderRaw), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(ReadShader("Shaders/Deferred/gbuffer.frag.spv", &gBufferFragmentShaderRaw), RendererResult::Success, RendererResult::Failure)

			// Create shader modules.
			VkShaderModule gBufferVertexShader, gBufferFragmentShader;
			CHECK_RESULT(CreateShaderModule(gBufferVertexShaderRaw, &gBufferVertexShader), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(CreateShaderModule(gBufferFragmentShaderRaw, &gBufferFragmentShader), RendererResult::Success, RendererResult::Failure)

			// Vertex stage creation information.
			VkPipelineShaderStageCreateInfo gBufferVertexCreateInfo = {};
			gBufferVertexCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			gBufferVertexCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			gBufferVertexCreateInfo.module = gBufferVertexShader;
			gBufferVertexCreateInfo.pName = "main";

			// Fragment stage creation information.
			VkPipelineShaderStageCreateInfo gBufferFragmentCreateInfo = {};
			gBufferFragmentCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			gBufferFragmentCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			gBufferFragmentCreateInfo.module = gBufferFragmentShader;
			gBufferFragmentCreateInfo.pName = "main";

			VkPipelineShaderStageCreateInfo shaderStages[] = {
				gBufferVertexCreateInfo,
				gBufferFragmentCreateInfo
			};

			/* Create graphics pipeline. */

			#pragma region Vertex Input Stage

			// Definitions of how data for a single vertex is a whole.
			VkVertexInputBindingDescription gBufferBindingDescription = {};
			gBufferBindingDescription.binding = 0;
			gBufferBindingDescription.stride = sizeof(Vertex);
			gBufferBindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			// How the data for an attribute is defined within the vertex.
			boost::array<VkVertexInputAttributeDescription, 3> gBufferAttributeDescriptions = {};

			// Position attribute.
			gBufferAttributeDescriptions[0].binding = 0;
			gBufferAttributeDescriptions[0].location = 0;
			gBufferAttributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			gBufferAttributeDescriptions[0].offset = offsetof(Vertex, _position);
			
			// Normal attribute.
			gBufferAttributeDescriptions[1].binding = 0;
			gBufferAttributeDescriptions[1].location = 1;
			gBufferAttributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			gBufferAttributeDescriptions[1].offset = offsetof(Vertex, _normal);

			// Texture coordinate attribute.
			gBufferAttributeDescriptions[2].binding = 0;
			gBufferAttributeDescriptions[2].location = 2;
			gBufferAttributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
			gBufferAttributeDescriptions[2].offset = offsetof(Vertex, _textureCoordinate);

			// Vertex input stage.
			VkPipelineVertexInputStateCreateInfo gBufferVertexInputCreateInfo = {};
			gBufferVertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			gBufferVertexInputCreateInfo.vertexBindingDescriptionCount = 1;
			gBufferVertexInputCreateInfo.pVertexBindingDescriptions = &gBufferBindingDescription;
			gBufferVertexInputCreateInfo.vertexAttributeDescriptionCount = static_cast<u32>(gBufferAttributeDescriptions.size());
			gBufferVertexInputCreateInfo.pVertexAttributeDescriptions = gBufferAttributeDescriptions.data();

			#pragma endregion
			#pragma region Input Assembly Stage

			// Input assembly stage.
			VkPipelineInputAssemblyStateCreateInfo gBufferInputAssemblyCreateInfo = {};
			gBufferInputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			gBufferInputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
			gBufferInputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

			#pragma endregion
			#pragma region Viewport and Scissors Stage

			// Viewport and scissors stage.

			// Create the viewport info.
			VkViewport gBufferViewport = {};
			gBufferViewport.x = 0.0f;
			gBufferViewport.y = 0.0f;
			gBufferViewport.width = static_cast<float>(_swapchainExtent.width);
			gBufferViewport.height = static_cast<float>(_swapchainExtent.height);
			gBufferViewport.minDepth = 0.0f;
			gBufferViewport.maxDepth = 1.0f;

			// Create the scissor info. (specifies what is visible)
			VkRect2D gBufferScissor = {};
			gBufferScissor.offset = { 0, 0 };
			gBufferScissor.extent = _swapchainExtent;

			// Create the viewport and scissors state info.
			VkPipelineViewportStateCreateInfo gBufferViewportCreateInfo = {};
			gBufferViewportCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			gBufferViewportCreateInfo.viewportCount = 1;
			gBufferViewportCreateInfo.pViewports = &gBufferViewport;
			gBufferViewportCreateInfo.scissorCount = 1;
			gBufferViewportCreateInfo.pScissors = &gBufferScissor;

			#pragma endregion
			#pragma region Rasterization Stage

			// Rasterization stage.
			VkPipelineRasterizationStateCreateInfo gBufferRasterCreateInfo = {};
			gBufferRasterCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			gBufferRasterCreateInfo.depthClampEnable = VK_FALSE;
			gBufferRasterCreateInfo.rasterizerDiscardEnable = VK_FALSE;
			gBufferRasterCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
			gBufferRasterCreateInfo.lineWidth = 1.0f;
			gBufferRasterCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
			gBufferRasterCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
			gBufferRasterCreateInfo.depthBiasEnable = VK_FALSE;

			#pragma endregion
			#pragma region Multisampling Stage

			// Multisampling stage.
			VkPipelineMultisampleStateCreateInfo gBufferMultisampleCreateInfo = {};
			gBufferMultisampleCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			gBufferMultisampleCreateInfo.sampleShadingEnable = VK_FALSE;
			gBufferMultisampleCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

			#pragma endregion
			#pragma region Blending Stage

			// Blending attachment information.
			VkPipelineColorBlendAttachmentState gBufferBlendState = {};
			gBufferBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			gBufferBlendState.blendEnable = VK_FALSE;
			//gBufferBlendState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
			//gBufferBlendState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
			//gBufferBlendState.alphaBlendOp = VK_BLEND_OP_ADD;
			//gBufferBlendState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			//gBufferBlendState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			//gBufferBlendState.colorBlendOp = VK_BLEND_OP_ADD;

			// Using following equation for blending:
			// (VK_BLEND_FACTOR_SRC_ALPHA * newColor) + (VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA * oldColor)

			// Replicate blend state for all color attachments.
			boost::container::vector<VkPipelineColorBlendAttachmentState> gBufferBlendStates(3, gBufferBlendState);

			// Blending stage creation information.
			VkPipelineColorBlendStateCreateInfo gBufferBlendCreateInfo = {};
			gBufferBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			gBufferBlendCreateInfo.logicOpEnable = VK_FALSE;
			gBufferBlendCreateInfo.attachmentCount = static_cast<u32>(gBufferBlendStates.size());
			gBufferBlendCreateInfo.pAttachments = gBufferBlendStates.data();

			#pragma region Depth Stencil Stage

			// TODO: Setup depth stencil testing
			VkPipelineDepthStencilStateCreateInfo gBufferDepthCreateInfo = {};
			gBufferDepthCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			gBufferDepthCreateInfo.depthTestEnable = VK_TRUE;
			gBufferDepthCreateInfo.depthWriteEnable = VK_TRUE;
			gBufferDepthCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
			gBufferDepthCreateInfo.depthBoundsTestEnable = VK_FALSE;
			gBufferDepthCreateInfo.stencilTestEnable = VK_FALSE;

			#pragma endregion

			#pragma endregion
			#pragma region Pipeline Layout Stage

			// Assemble pipeline layouts into an array.
			boost::array<VkDescriptorSetLayout, 2> gBufferDescriptorSetLayouts = {
				_bufferDescriptorSetLayout, _samplerDescriptorSetLayout
			};

			// Pipeline layout stage creation information.
			VkPipelineLayoutCreateInfo gBufferLayoutCreateInfo = {};
			gBufferLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			gBufferLayoutCreateInfo.setLayoutCount = static_cast<u32>(gBufferDescriptorSetLayouts.size());
			gBufferLayoutCreateInfo.pSetLayouts = gBufferDescriptorSetLayouts.data();
			gBufferLayoutCreateInfo.pushConstantRangeCount = 1;
			gBufferLayoutCreateInfo.pPushConstantRanges = &_gBufferPushConstantRange;

			// Create pipeline layout.
			CHECK_RESULT(vkCreatePipelineLayout(_device._logical, &gBufferLayoutCreateInfo, nullptr, &_gBufferPipelineLayout), VK_SUCCESS, RendererResult::Failure)
			
			#pragma endregion

			// Create graphics pipeline information.
			VkGraphicsPipelineCreateInfo gBufferPipelineCreateInfo = {};
			gBufferPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			gBufferPipelineCreateInfo.stageCount = 2;
			gBufferPipelineCreateInfo.pStages = shaderStages;
			gBufferPipelineCreateInfo.pVertexInputState = &gBufferVertexInputCreateInfo;
			gBufferPipelineCreateInfo.pInputAssemblyState = &gBufferInputAssemblyCreateInfo;
			gBufferPipelineCreateInfo.pViewportState = &gBufferViewportCreateInfo;
			gBufferPipelineCreateInfo.pDynamicState = nullptr;
			gBufferPipelineCreateInfo.pRasterizationState = &gBufferRasterCreateInfo;
			gBufferPipelineCreateInfo.pMultisampleState = &gBufferMultisampleCreateInfo;
			gBufferPipelineCreateInfo.pColorBlendState = &gBufferBlendCreateInfo;
			gBufferPipelineCreateInfo.pDepthStencilState = &gBufferDepthCreateInfo;
			gBufferPipelineCreateInfo.layout = _gBufferPipelineLayout;
			gBufferPipelineCreateInfo.renderPass = _renderPass;							// Render pass it is compatible with.
			gBufferPipelineCreateInfo.subpass = 0;											// Subpass of the render pass that it will use.

			// Derivative graphics pipelines (use these attributes to base a pipeline onto another).
			gBufferPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			gBufferPipelineCreateInfo.basePipelineIndex = -1;

			CHECK_RESULT(vkCreateGraphicsPipelines(_device._logical, VK_NULL_HANDLE, 1, &gBufferPipelineCreateInfo, nullptr, &_gBufferPipeline), VK_SUCCESS, RendererResult::Failure)
			
			// Destroy shader modules. (no longer needed after creating the pipeline)
			vkDestroyShaderModule(_device._logical, gBufferFragmentShader, nullptr);
			vkDestroyShaderModule(_device._logical, gBufferVertexShader, nullptr);

			// LIGHTING GRAPHICS PIPELINE

			boost::container::vector<char> lightingVertexShaderRaw, lightingFragmentShaderRaw;
			CHECK_RESULT(ReadShader("Shaders/Deferred/lighting.vert.spv", &lightingVertexShaderRaw), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(ReadShader("Shaders/Deferred/lighting.frag.spv", &lightingFragmentShaderRaw), RendererResult::Success, RendererResult::Failure)

			// Create shader modules.
			VkShaderModule lightingVertexShader, lightingFragmentShader;
			CHECK_RESULT(CreateShaderModule(lightingVertexShaderRaw, &lightingVertexShader), RendererResult::Success, RendererResult::Failure)
			CHECK_RESULT(CreateShaderModule(lightingFragmentShaderRaw, &lightingFragmentShader), RendererResult::Success, RendererResult::Failure)
			
			// Vertex stage creation information.
			VkPipelineShaderStageCreateInfo lightingVertexCreateInfo = {};
			lightingVertexCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			lightingVertexCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
			lightingVertexCreateInfo.module = lightingVertexShader;
			lightingVertexCreateInfo.pName = "main";

			// Fragment stage creation information.
			VkPipelineShaderStageCreateInfo lightingFragmentCreateInfo = {};
			lightingFragmentCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
			lightingFragmentCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
			lightingFragmentCreateInfo.module = lightingFragmentShader;
			lightingFragmentCreateInfo.pName = "main";

			// Assemble shader stages into an array.
			boost::array<VkPipelineShaderStageCreateInfo, 2> lightingShaderStages = {
				lightingVertexCreateInfo, lightingFragmentCreateInfo
			};

			// Remove vertex data for lighting pipeline.
			gBufferVertexInputCreateInfo.vertexBindingDescriptionCount = 0;
			gBufferVertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
			gBufferVertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
			gBufferVertexInputCreateInfo.pVertexBindingDescriptions = nullptr;

			// Remove writing to depth buffer.
			gBufferDepthCreateInfo.depthWriteEnable = VK_FALSE;

			// Blending attachment information.
			VkPipelineColorBlendAttachmentState lightingBlendState = {};
			lightingBlendState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
				VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
			lightingBlendState.blendEnable = VK_FALSE;

			// Blending stage creation information.
			VkPipelineColorBlendStateCreateInfo lightingBlendCreateInfo = {};
			lightingBlendCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			lightingBlendCreateInfo.logicOpEnable = VK_FALSE;
			lightingBlendCreateInfo.attachmentCount = 1;
			lightingBlendCreateInfo.pAttachments = &lightingBlendState;

			// Assemble descriptor layouts into an array.
			boost::array<VkDescriptorSetLayout, 2> lightingDescriptorSetLayouts = {
				_gBufferDescriptorSetLayout, _lightingDescriptorSetLayout
			};

			// Pipeline layout stage creation information.
			VkPipelineLayoutCreateInfo lightingPipelineLayoutCreateInfo = {};
			lightingPipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			lightingPipelineLayoutCreateInfo.setLayoutCount = static_cast<u32>(lightingDescriptorSetLayouts.size());
			lightingPipelineLayoutCreateInfo.pSetLayouts = lightingDescriptorSetLayouts.data();
			lightingPipelineLayoutCreateInfo.pushConstantRangeCount = 1;
			lightingPipelineLayoutCreateInfo.pPushConstantRanges = &_lightingPushConstantRange;

			// Create pipeline layout.
			CHECK_RESULT(vkCreatePipelineLayout(_device._logical, &lightingPipelineLayoutCreateInfo, nullptr, &_lightingPipelineLayout), VK_SUCCESS, RendererResult::Failure)

			// Modify graphics pipeline information.
			gBufferPipelineCreateInfo.stageCount = static_cast<u32>(lightingShaderStages.size());
			gBufferPipelineCreateInfo.pStages = lightingShaderStages.data();
			gBufferPipelineCreateInfo.pColorBlendState = &lightingBlendCreateInfo;
			gBufferPipelineCreateInfo.layout = _lightingPipelineLayout;
			gBufferPipelineCreateInfo.subpass = 1;

			// Create lighting graphics pipeline.
			CHECK_RESULT(vkCreateGraphicsPipelines(_device._logical, VK_NULL_HANDLE, 1, &gBufferPipelineCreateInfo, nullptr, &_lightingPipeline), VK_SUCCESS, RendererResult::Failure)

			// Destroy shader modules. (no longer needed after creating the pipeline)
			vkDestroyShaderModule(_device._logical, lightingFragmentShader, nullptr);
			vkDestroyShaderModule(_device._logical, lightingVertexShader, nullptr);

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateGBuffer()
		{
			// Resize the buffers to match with the swapchain output.
			_colorBuffer.resize(_swapchainImages.size());
			_depthBuffer.resize(_swapchainImages.size());
			_normalBuffer.resize(_swapchainImages.size());
			_positionBuffer.resize(_swapchainImages.size());

			// Choose the best formats for each buffer.
			_colorFormat = ChooseBestSupportedFormat({ VK_FORMAT_B8G8R8A8_UNORM }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
			_depthFormat = ChooseBestSupportedFormat({ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
				VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
			_normalFormat = ChooseBestSupportedFormat({ VK_FORMAT_R16G16B16A16_SFLOAT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
			_positionFormat = ChooseBestSupportedFormat({ VK_FORMAT_R16G16B16A16_SFLOAT }, VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_COLOR_ATTACHMENT_BIT);
			
			// Create the color buffer.
			for (usize i = 0; i < _colorBuffer.size(); ++i)
			{
				CHECK_RESULT(CreateImage(_swapchainExtent.width, _swapchainExtent.height, _colorFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, &_colorBuffer[i]._raw), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(AllocateImage(_colorBuffer[i]._raw, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_colorBuffer[i]._memory), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(CreateImageView(_colorBuffer[i]._raw, _colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, &_colorBuffer[i]._view), RendererResult::Success, RendererResult::Failure)
			}

			// Create the depth buffer.
			for (usize i = 0; i < _depthBuffer.size(); ++i)
			{
				CHECK_RESULT(CreateImage(_swapchainExtent.width, _swapchainExtent.height, _depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, &_depthBuffer[i]._raw), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(AllocateImage(_depthBuffer[i]._raw, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_depthBuffer[i]._memory), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(CreateImageView(_depthBuffer[i]._raw, _depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &_depthBuffer[i]._view), RendererResult::Success, RendererResult::Failure)
			}

			// Create the normal buffer.
			for (usize i = 0; i < _normalBuffer.size(); ++i)
			{
				CHECK_RESULT(CreateImage(_swapchainExtent.width, _swapchainExtent.height, _normalFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, &_normalBuffer[i]._raw), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(AllocateImage(_normalBuffer[i]._raw, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_normalBuffer[i]._memory), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(CreateImageView(_normalBuffer[i]._raw, _normalFormat, VK_IMAGE_ASPECT_COLOR_BIT, &_normalBuffer[i]._view), RendererResult::Success, RendererResult::Failure)
			}

			// Create the position buffer.
			for (usize i = 0; i < _positionBuffer.size(); ++i)
			{
				CHECK_RESULT(CreateImage(_swapchainExtent.width, _swapchainExtent.height, _positionFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT, &_positionBuffer[i]._raw), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(AllocateImage(_positionBuffer[i]._raw, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &_positionBuffer[i]._memory), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(CreateImageView(_positionBuffer[i]._raw, _positionFormat, VK_IMAGE_ASPECT_COLOR_BIT, &_positionBuffer[i]._view), RendererResult::Success, RendererResult::Failure)
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateFramebuffers()
		{
			_swapchainFramebuffers.resize(_swapchainImages.size());
			for (usize i = 0; i < _swapchainFramebuffers.size(); ++i)
			{
				boost::array<VkImageView, 5> attachments = {
					_colorBuffer[i]._view, _depthBuffer[i]._view, _normalBuffer[i]._view, _positionBuffer[i]._view, _swapchainImages[i]._view
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

			CHECK_RESULT(vkCreateCommandPool(_device._logical, &createInfo, nullptr, &_graphicsPool), VK_SUCCESS, RendererResult::Failure)

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
				//_transferPool = _graphicsPool;
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateCommandBuffers()
		{
			VkCommandBufferAllocateInfo allocateInfo = {};
			allocateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
			allocateInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

			// Create command buffers for graphical operations.
			_commandBuffers.resize(_swapchainImages.size());

			// Configure allocation for specific command buffer set.
			allocateInfo.commandPool = _graphicsPool;
			allocateInfo.commandBufferCount = static_cast<u32>(_commandBuffers.size());

			CHECK_RESULT(vkAllocateCommandBuffers(_device._logical, &allocateInfo, _commandBuffers.data()), VK_SUCCESS, RendererResult::Failure)
		
			// Create command buffer for transfer operations.
			allocateInfo.commandPool = _transferPool;
			allocateInfo.commandBufferCount = 1;
			CHECK_RESULT(vkAllocateCommandBuffers(_device._logical, &allocateInfo, &_transferBuffer), VK_SUCCESS, RendererResult::Failure)

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateUniformBuffers()
		{
			VkDeviceSize cameraUniformBufferSize = sizeof(CameraUniform);
			VkDeviceSize lightingUniformBufferSize = sizeof(LightingUniform);
			VkDeviceSize materialUniformBufferSize = GetAlignedSize(sizeof(MaterialUniform), _minUniformBufferAlignment) * MAX_MATERIALS;

			// Create one vertex uniform buffer for each swapchain image.
			_cameraUniformBuffers.resize(_swapchainImages.size());
			_cameraUniformBuffersMemory.resize(_swapchainImages.size());
			for (usize i = 0; i < _cameraUniformBuffers.size(); ++i)
			{
				CHECK_RESULT(CreateBuffer(cameraUniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &_cameraUniformBuffers[i]), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(AllocateBuffer(_cameraUniformBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &_cameraUniformBuffersMemory[i]), RendererResult::Success, RendererResult::Failure)
			}

			// Create one fragment dynamic uniform buffer for each swapchain image.
			_materialUniformBuffers.resize(_swapchainImages.size());
			_materialUniformBuffersMemory.resize(_swapchainImages.size());
			for (usize i = 0; i < _materialUniformBuffers.size(); ++i)
			{
				CHECK_RESULT(CreateBuffer(materialUniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &_materialUniformBuffers[i]), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(AllocateBuffer(_materialUniformBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &_materialUniformBuffersMemory[i]), RendererResult::Success, RendererResult::Failure)
			}

			// Create one fragment uniform buffer for each swapchain image.
			_lightingUniformBuffers.resize(_swapchainImages.size());
			_lightingUniformBuffersMemory.resize(_swapchainImages.size());
			for (usize i = 0; i < _lightingUniformBuffers.size(); ++i)
			{
				CHECK_RESULT(CreateBuffer(lightingUniformBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, &_lightingUniformBuffers[i]), RendererResult::Success, RendererResult::Failure)
				CHECK_RESULT(AllocateBuffer(_lightingUniformBuffers[i], VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, &_lightingUniformBuffersMemory[i]), RendererResult::Success, RendererResult::Failure)
			}

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateDescriptorPools()
		{
			// BUFFER DESCRIPTOR POOL (G-BUFFER PIPELINE)

			// Information about pool for the camera uniform descriptors.
			VkDescriptorPoolSize cameraPoolSize = {};
			cameraPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			cameraPoolSize.descriptorCount = static_cast<u32>(_swapchainImages.size());

			// Information about pool for the material uniform descriptors.
			VkDescriptorPoolSize materialPoolSize = {};
			materialPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
			materialPoolSize.descriptorCount = static_cast<u32>(_swapchainImages.size());

			// Assemble pool sizes into an array.
			boost::array<VkDescriptorPoolSize, 2> poolSizes = {
				cameraPoolSize, materialPoolSize
			};

			// Buffer descriptor pool creation information.
			VkDescriptorPoolCreateInfo bufferPoolCreateInfo = {};
			bufferPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			bufferPoolCreateInfo.maxSets = static_cast<u32>(_swapchainImages.size());
			bufferPoolCreateInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
			bufferPoolCreateInfo.pPoolSizes = poolSizes.data();

			CHECK_RESULT(vkCreateDescriptorPool(_device._logical, &bufferPoolCreateInfo, nullptr, &_bufferDescriptorPool), VK_SUCCESS, RendererResult::Failure)
			
			// SAMPLER DESCRIPTOR POOL (G-BUFFER PIPELINE)

			// Information about pool for the texture sampler descriptors.
			VkDescriptorPoolSize samplerPoolSize = {};
			samplerPoolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
			samplerPoolSize.descriptorCount = MAX_TEXTURES;

			// Sampler descriptor pool creation information.
			VkDescriptorPoolCreateInfo samplerPoolCreateInfo = {};
			samplerPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			samplerPoolCreateInfo.maxSets = MAX_TEXTURES;
			samplerPoolCreateInfo.poolSizeCount = 1;
			samplerPoolCreateInfo.pPoolSizes = &samplerPoolSize;
			samplerPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

			CHECK_RESULT(vkCreateDescriptorPool(_device._logical, &samplerPoolCreateInfo, nullptr, &_samplerDescriptorPool), VK_SUCCESS, RendererResult::Failure)

			// G-BUFFER DESCRIPTOR POOL (LIGHTING PIPELINE)

			// Information about pool for the color image descriptors.
			VkDescriptorPoolSize colorPoolSize = {};
			colorPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			colorPoolSize.descriptorCount = static_cast<u32>(_colorBuffer.size());

			// Information about pool for the depth image descriptors.
			VkDescriptorPoolSize depthPoolSize = {};
			depthPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			depthPoolSize.descriptorCount = static_cast<u32>(_depthBuffer.size());

			// Information about pool for the normal image descriptors.
			VkDescriptorPoolSize normalPoolSize = {};
			normalPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			normalPoolSize.descriptorCount = static_cast<u32>(_normalBuffer.size());

			// Information about pool for the position image descriptors.
			VkDescriptorPoolSize positionPoolSize = {};
			positionPoolSize.type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
			positionPoolSize.descriptorCount = static_cast<u32>(_positionBuffer.size());

			// Assemble pool sizes into an array.
			boost::array<VkDescriptorPoolSize, 4> gBufferPoolSizes = {
				colorPoolSize, depthPoolSize, normalPoolSize, positionPoolSize
			};

			// G-buffer descriptor pool creation information.
			VkDescriptorPoolCreateInfo gBufferPoolCreateInfo = {};
			gBufferPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			gBufferPoolCreateInfo.maxSets = _swapchainImages.size();
			gBufferPoolCreateInfo.poolSizeCount = static_cast<u32>(gBufferPoolSizes.size());
			gBufferPoolCreateInfo.pPoolSizes = gBufferPoolSizes.data();

			CHECK_RESULT(vkCreateDescriptorPool(_device._logical, &gBufferPoolCreateInfo, nullptr, &_gBufferDescriptorPool), VK_SUCCESS, RendererResult::Failure)

			// LIGHTING DESCRIPTOR POOL (LIGHTING PIPELINE)

			// Information about pool for the lighting descriptors.
			VkDescriptorPoolSize lightingPoolSize = {};
			lightingPoolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			lightingPoolSize.descriptorCount = static_cast<u32>(_swapchainImages.size());

			// Lighting descriptor pool creation information.
			VkDescriptorPoolCreateInfo lightingPoolCreateInfo = {};
			lightingPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			lightingPoolCreateInfo.maxSets = _swapchainImages.size();
			lightingPoolCreateInfo.poolSizeCount = 1;
			lightingPoolCreateInfo.pPoolSizes = &lightingPoolSize;

			CHECK_RESULT(vkCreateDescriptorPool(_device._logical, &lightingPoolCreateInfo, nullptr, &_lightingDescriptorPool), VK_SUCCESS, RendererResult::Failure)

			return RendererResult::Success;
		}

		RendererResult Renderer::CreateDescriptorSets()
		{
			// BUFFER DESCRIPTOR SETS (G-BUFFER PIPELINE)

			// Setup a vector with layouts for each descriptor set created.
			boost::container::vector<VkDescriptorSetLayout> bufferSetLayouts(_swapchainImages.size(), _bufferDescriptorSetLayout);

			// Resize descriptor sets to match the uniform buffers they describe.
			_bufferDescriptorSets.resize(_swapchainImages.size());

			// Descriptor set allocation information.
			VkDescriptorSetAllocateInfo bufferAllocateInfo = {};
			bufferAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			bufferAllocateInfo.descriptorPool = _bufferDescriptorPool;
			bufferAllocateInfo.descriptorSetCount = static_cast<u32>(_swapchainImages.size());
			bufferAllocateInfo.pSetLayouts = bufferSetLayouts.data();

			// Allocate descriptor sets.
			CHECK_RESULT(vkAllocateDescriptorSets(_device._logical, &bufferAllocateInfo, _bufferDescriptorSets.data()), VK_SUCCESS, RendererResult::Failure)

			// Create the vectors to store descriptor set update information.
			boost::container::vector<VkDescriptorBufferInfo> bufferDescriptorBuffer;
			boost::container::vector<VkWriteDescriptorSet> bufferDescriptorWrite;

			// Resize vectors to fit every single descriptor.
			bufferDescriptorBuffer.resize(2 * _bufferDescriptorSets.size());
			bufferDescriptorWrite.resize(2 * _bufferDescriptorSets.size());

			// Fill the information of the bindings between descriptor sets and uniform buffers.
			for (usize i = 0; i < _bufferDescriptorSets.size(); ++i)
			{
				// Camera descriptor buffer information.
				bufferDescriptorBuffer[i].buffer = _cameraUniformBuffers[i];
				bufferDescriptorBuffer[i].offset = 0;
				bufferDescriptorBuffer[i].range = sizeof(CameraUniform);

				// Camera write descriptor write information.
				bufferDescriptorWrite[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				bufferDescriptorWrite[i].dstSet = _bufferDescriptorSets[i];
				bufferDescriptorWrite[i].dstBinding = 0;
				bufferDescriptorWrite[i].dstArrayElement = 0;
				bufferDescriptorWrite[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				bufferDescriptorWrite[i].descriptorCount = 1;
				bufferDescriptorWrite[i].pBufferInfo = &bufferDescriptorBuffer[i];

				// Material descriptor buffer information.
				bufferDescriptorBuffer[i + _bufferDescriptorSets.size()].buffer = _materialUniformBuffers[i];
				bufferDescriptorBuffer[i + _bufferDescriptorSets.size()].offset = 0;
				bufferDescriptorBuffer[i + _bufferDescriptorSets.size()].range = GetAlignedSize(sizeof(MaterialUniform), _minUniformBufferAlignment);

				// Material descriptor write information.
				bufferDescriptorWrite[i + _bufferDescriptorSets.size()].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				bufferDescriptorWrite[i + _bufferDescriptorSets.size()].dstSet = _bufferDescriptorSets[i];
				bufferDescriptorWrite[i + _bufferDescriptorSets.size()].dstBinding = 1;
				bufferDescriptorWrite[i + _bufferDescriptorSets.size()].dstArrayElement = 0;
				bufferDescriptorWrite[i + _bufferDescriptorSets.size()].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC;
				bufferDescriptorWrite[i + _bufferDescriptorSets.size()].descriptorCount = 1;
				bufferDescriptorWrite[i + _bufferDescriptorSets.size()].pBufferInfo = &bufferDescriptorBuffer[i + _bufferDescriptorSets.size()];
			}

			// Update the descriptor sets with the filled information.
			vkUpdateDescriptorSets(_device._logical, bufferDescriptorWrite.size(), bufferDescriptorWrite.data(), 0, nullptr);

			// G-BUFFER DESCRIPTOR SETS (LIGHTING PIPELINE)

			// Setup a vector with layouts for each descriptor set created.
			boost::container::vector<VkDescriptorSetLayout> gBufferSetLayouts(_swapchainImages.size(), _gBufferDescriptorSetLayout);

			// Resize descriptor sets to match the images they describe.
			_gBufferDescriptorSets.resize(_swapchainImages.size());

			// Descriptor set allocation information.
			VkDescriptorSetAllocateInfo gBufferAllocateInfo = {};
			gBufferAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			gBufferAllocateInfo.descriptorPool = _gBufferDescriptorPool;
			gBufferAllocateInfo.descriptorSetCount = _swapchainImages.size();
			gBufferAllocateInfo.pSetLayouts = gBufferSetLayouts.data();

			// Allocate descriptor sets.
			CHECK_RESULT(vkAllocateDescriptorSets(_device._logical, &gBufferAllocateInfo, _gBufferDescriptorSets.data()), VK_SUCCESS, RendererResult::Failure)

			// Create the vectors to store descriptor set update information.
			boost::container::vector<VkDescriptorImageInfo> gBufferDescriptorImage;
			boost::container::vector<VkWriteDescriptorSet> gBufferDescriptorWrite;

			// Resize vectors to fit every single descriptor.
			gBufferDescriptorImage.resize(4 * _gBufferDescriptorSets.size());
			gBufferDescriptorWrite.resize(4 * _gBufferDescriptorSets.size());

			// Fill the information of the bindings between descriptor sets and images.
			for (usize i = 0; i < _gBufferDescriptorSets.size(); ++i)
			{
				// Color descriptor image information.
				gBufferDescriptorImage[i].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				gBufferDescriptorImage[i].imageView = _colorBuffer[i]._view;
				gBufferDescriptorImage[i].sampler = VK_NULL_HANDLE;

				// Colour descriptor write information.
				gBufferDescriptorWrite[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				gBufferDescriptorWrite[i].dstSet = _gBufferDescriptorSets[i];
				gBufferDescriptorWrite[i].dstBinding = 0;
				gBufferDescriptorWrite[i].dstArrayElement = 0;
				gBufferDescriptorWrite[i].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
				gBufferDescriptorWrite[i].descriptorCount = 1;
				gBufferDescriptorWrite[i].pImageInfo = &gBufferDescriptorImage[i];

				// Depth descriptor image information.
				gBufferDescriptorImage[i + _gBufferDescriptorSets.size()].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				gBufferDescriptorImage[i + _gBufferDescriptorSets.size()].imageView = _depthBuffer[i]._view;
				gBufferDescriptorImage[i + _gBufferDescriptorSets.size()].sampler = VK_NULL_HANDLE;

				// Depth descriptor write information.
				gBufferDescriptorWrite[i + _gBufferDescriptorSets.size()].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				gBufferDescriptorWrite[i + _gBufferDescriptorSets.size()].dstSet = _gBufferDescriptorSets[i];
				gBufferDescriptorWrite[i + _gBufferDescriptorSets.size()].dstBinding = 1;
				gBufferDescriptorWrite[i + _gBufferDescriptorSets.size()].dstArrayElement = 0;
				gBufferDescriptorWrite[i + _gBufferDescriptorSets.size()].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
				gBufferDescriptorWrite[i + _gBufferDescriptorSets.size()].descriptorCount = 1;
				gBufferDescriptorWrite[i + _gBufferDescriptorSets.size()].pImageInfo = &gBufferDescriptorImage[i + _gBufferDescriptorSets.size()];

				// Normal descriptor image information.
				gBufferDescriptorImage[i + 2 * _gBufferDescriptorSets.size()].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				gBufferDescriptorImage[i + 2 * _gBufferDescriptorSets.size()].imageView = _normalBuffer[i]._view;
				gBufferDescriptorImage[i + 2 * _gBufferDescriptorSets.size()].sampler = VK_NULL_HANDLE;

				// Normal descriptor write information.
				gBufferDescriptorWrite[i + 2 * _gBufferDescriptorSets.size()].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				gBufferDescriptorWrite[i + 2 * _gBufferDescriptorSets.size()].dstSet = _gBufferDescriptorSets[i];
				gBufferDescriptorWrite[i + 2 * _gBufferDescriptorSets.size()].dstBinding = 2;
				gBufferDescriptorWrite[i + 2 * _gBufferDescriptorSets.size()].dstArrayElement = 0;
				gBufferDescriptorWrite[i + 2 * _gBufferDescriptorSets.size()].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
				gBufferDescriptorWrite[i + 2 * _gBufferDescriptorSets.size()].descriptorCount = 1;
				gBufferDescriptorWrite[i + 2 * _gBufferDescriptorSets.size()].pImageInfo = &gBufferDescriptorImage[i + 2 * _gBufferDescriptorSets.size()];

				// Position descriptor image information.
				gBufferDescriptorImage[i + 3 * _gBufferDescriptorSets.size()].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
				gBufferDescriptorImage[i + 3 * _gBufferDescriptorSets.size()].imageView = _positionBuffer[i]._view;
				gBufferDescriptorImage[i + 3 * _gBufferDescriptorSets.size()].sampler = VK_NULL_HANDLE;

				// Position descriptor write information.
				gBufferDescriptorWrite[i + 3 * _gBufferDescriptorSets.size()].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				gBufferDescriptorWrite[i + 3 * _gBufferDescriptorSets.size()].dstSet = _gBufferDescriptorSets[i];
				gBufferDescriptorWrite[i + 3 * _gBufferDescriptorSets.size()].dstBinding = 3;
				gBufferDescriptorWrite[i + 3 * _gBufferDescriptorSets.size()].dstArrayElement = 0;
				gBufferDescriptorWrite[i + 3 * _gBufferDescriptorSets.size()].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
				gBufferDescriptorWrite[i + 3 * _gBufferDescriptorSets.size()].descriptorCount = 1;
				gBufferDescriptorWrite[i + 3 * _gBufferDescriptorSets.size()].pImageInfo = &gBufferDescriptorImage[i + 3 * _gBufferDescriptorSets.size()];
			}

			// Update the descriptor sets with the filled information.
			vkUpdateDescriptorSets(_device._logical, static_cast<u32>(gBufferDescriptorWrite.size()), gBufferDescriptorWrite.data(), 0, nullptr);

			// LIGHTING DESCRIPTOR SETS (LIGHTING PIPELINE)

			// Setup a vector with layouts for each descriptor set created.
			boost::container::vector<VkDescriptorSetLayout> lightingSetLayouts(_swapchainImages.size(), _lightingDescriptorSetLayout);

			// Resize descriptor sets to match the images they describe.
			_lightingDescriptorSets.resize(_swapchainImages.size());

			// Descriptor set allocation information.
			VkDescriptorSetAllocateInfo lightingAllocateInfo = {};
			lightingAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			lightingAllocateInfo.descriptorPool = _lightingDescriptorPool;
			lightingAllocateInfo.descriptorSetCount = _swapchainImages.size();
			lightingAllocateInfo.pSetLayouts = lightingSetLayouts.data();

			// Allocate descriptor sets.
			CHECK_RESULT(vkAllocateDescriptorSets(_device._logical, &lightingAllocateInfo, _lightingDescriptorSets.data()), VK_SUCCESS, RendererResult::Failure)

			// Create the vectors to store descriptor set update information.
			boost::container::vector<VkDescriptorBufferInfo> lightingDescriptorBuffer;
			boost::container::vector<VkWriteDescriptorSet> lightingDescriptorWrite;

			// Resize vectors to fit every single descriptor.
			lightingDescriptorBuffer.resize(_lightingDescriptorSets.size());
			lightingDescriptorWrite.resize(_lightingDescriptorSets.size());

			// Fill the information of the bindings between descriptor sets and buffers.
			for (usize i = 0; i < _lightingDescriptorSets.size(); ++i)
			{
				// Fragment descriptor buffer information.
				lightingDescriptorBuffer[i].buffer = _lightingUniformBuffers[i];
				lightingDescriptorBuffer[i].offset = 0;
				lightingDescriptorBuffer[i].range = sizeof(LightingUniform);

				// Fragment write descriptor write information.
				lightingDescriptorWrite[i].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				lightingDescriptorWrite[i].dstSet = _lightingDescriptorSets[i];
				lightingDescriptorWrite[i].dstBinding = 0;
				lightingDescriptorWrite[i].dstArrayElement = 0;
				lightingDescriptorWrite[i].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				lightingDescriptorWrite[i].descriptorCount = 1;
				lightingDescriptorWrite[i].pBufferInfo = &lightingDescriptorBuffer[i];
			}

			// Update the descriptor sets with the filled information.
			vkUpdateDescriptorSets(_device._logical, lightingDescriptorWrite.size(), lightingDescriptorWrite.data(), 0, nullptr);

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

		RendererResult Renderer::CreateTextureSampler()
		{
			// Sampler creation information.
			VkSamplerCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
			createInfo.magFilter = VK_FILTER_LINEAR;
			createInfo.minFilter = VK_FILTER_LINEAR;
			createInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
			createInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
			createInfo.unnormalizedCoordinates = VK_FALSE;
			createInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
			createInfo.mipLodBias = 0.0f;
			createInfo.minLod = 0.0f;
			createInfo.maxLod = 0.0f;
			createInfo.anisotropyEnable = VK_TRUE;
			createInfo.maxAnisotropy = 16.0f;

			CHECK_RESULT(vkCreateSampler(_device._logical, &createInfo, nullptr, &_textureSampler), VK_SUCCESS, RendererResult::Failure)
			return RendererResult::Success;
		}

		RendererResult Renderer::RecordCommands(usize offset, usize size)
		{
			ASSERT(offset >= 0);
			ASSERT(size <= _commandBuffers.size());

			// Information about how to begin a command buffer.
			VkCommandBufferBeginInfo bufferBeginInfo = {};
			bufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

			// Define the values to clear the framebuffer attachments with.
			boost::array<VkClearValue, 5> clearValues = {};
			clearValues[0].color = Math::Colors::Black;
			clearValues[1].depthStencil.depth = 1.0f;
			clearValues[2].color = Math::Colors::Black;
			clearValues[3].color = Math::Colors::Black;
			clearValues[4].color = Math::Colors::Black;

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
				CHECK_RESULT(vkResetCommandBuffer(_commandBuffers[i], 0), VK_SUCCESS, RendererResult::Failure)
				CHECK_RESULT(vkBeginCommandBuffer(_commandBuffers[i], &bufferBeginInfo), VK_SUCCESS, RendererResult::Failure)

				// Acquire lock to stop transfers from taking place.
				_transferMutex.lock();

				// Acquire images released by the transfer queue.
                _releasedImages.consume_all([this, i](VkImage image) {
                    TransitionImageLayout(_commandBuffers[i], image, _transferQueueFamily, _graphicsQueueFamily, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                        VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 0, VK_ACCESS_SHADER_READ_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);
                    });

				vkCmdBeginRenderPass(_commandBuffers[i], &passBeginInfo, VK_SUBPASS_CONTENTS_INLINE);
				
				if (_entitiesToRender.size() > 0)
				{
					// Bind the G-Buffer graphics pipeline to render geometry.
					vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _gBufferPipeline);

					usize j = 0;
					for (auto& entity : _entitiesToRender)
					{
						// Retrieve the entity information.
						const EntityInfo& entityInfo = entity.second;

						// Create G-buffer push constant from object and camera information.
						GBufferPush gBufferPush = {};
						gBufferPush._model = entityInfo._transformComponent ? entityInfo._transformComponent->GetModel() : Math::Matrix::Identity();
						gBufferPush._view = _activeCamera ? _activeCamera->GetView() : Math::Matrix::Identity();

						// Push constants into the shaders.
						vkCmdPushConstants(_commandBuffers[i], _gBufferPipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(GBufferPush), &gBufferPush);

						for (auto& renderableInfo : entityInfo._renderables)
						{
							// Acquire required information from the entities to be rendered.
							VkBuffer vertexBuffer = renderableInfo._vertexBuffer;
							VkBuffer indexBuffer = renderableInfo._indexBuffer;
							u32 indexCount = renderableInfo._indexCount;
							VkImage diffuseImage = renderableInfo._diffuseImage;
							VkDeviceSize offsets[] = { 0 };

							// Bind vertex and index buffers.
							vkCmdBindVertexBuffers(_commandBuffers[i], 0, 1, &vertexBuffer, offsets);
							vkCmdBindIndexBuffer(_commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);

							// Configure dynamic offsets for dynamic buffers.
							u32 dynamicOffset = static_cast<u32>(GetAlignedSize(sizeof(MaterialUniform), _minUniformBufferAlignment)) * j;

							// Assemble descriptor sets into a single array.
							boost::array<VkDescriptorSet, 2> gBufferDescriptorSets = {
								_bufferDescriptorSets[i], _textureDescriptorSets[diffuseImage]
							};

							// Bind descriptor sets.
							vkCmdBindDescriptorSets(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _gBufferPipelineLayout,
								0, gBufferDescriptorSets.size(), gBufferDescriptorSets.data(), 1, &dynamicOffset);

							// Add rendering commands to the buffer.
							vkCmdDrawIndexed(_commandBuffers[i], indexCount, 1, 0, 0, 0);
							j++;
						}
					}
				}

				// Release mutex after recording commands for this frame.
				_transferMutex.unlock();

				// Start the second subpass and bind the lighting graphics pipeline.
				vkCmdNextSubpass(_commandBuffers[i], VK_SUBPASS_CONTENTS_INLINE);
				vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _lightingPipeline);

				// Create lighting push constant from object and camera information.
				LightingPush lightingPush = {};
				lightingPush._eyePosition = _activeCamera ? _activeCamera->GetTransform()->GetPosition() : Math::Vector3(0.0f);

				// Push constants into the shaders.
				vkCmdPushConstants(_commandBuffers[i], _lightingPipelineLayout, VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(LightingPush), &lightingPush);

				// Assemble descriptor sets into a single array.
				boost::array<VkDescriptorSet, 2> lightingDescriptorSets = {
					_gBufferDescriptorSets[i], _lightingDescriptorSets[i]
				};

				// Bind the G-buffer descriptor sets.
				vkCmdBindDescriptorSets(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _lightingPipelineLayout,
					0, static_cast<u32>(lightingDescriptorSets.size()), lightingDescriptorSets.data(), 0, nullptr);

				// Request drawing operation to force fragment shader execution.
				vkCmdDraw(_commandBuffers[i], 3, 1, 0, 0);

				vkCmdEndRenderPass(_commandBuffers[i]);

				CHECK_RESULT(vkEndCommandBuffer(_commandBuffers[i]), VK_SUCCESS, RendererResult::Failure)
			}
			
			return RendererResult::Success;
		}

		void Renderer::UpdateDirectionalLight(LightingUniform::FragmentDirectionalLight* dstLight, const boost::shared_ptr<Entities::DirectionalLight>& srcLight)
		{
			dstLight->_base._color = srcLight->GetColor();
			dstLight->_base._ambientStrength = srcLight->GetAmbientStrength();
			dstLight->_base._diffuseStrength = srcLight->GetDiffuseStrength();
			dstLight->_direction = srcLight->GetDirection();
		}

		void Renderer::UpdatePointLight(LightingUniform::FragmentPointLight* dstLight, const boost::shared_ptr<Entities::PointLight>& srcLight)
		{
			dstLight->_base._color = srcLight->GetColor();
			dstLight->_base._ambientStrength = srcLight->GetAmbientStrength();
			dstLight->_base._diffuseStrength = srcLight->GetDiffuseStrength();
			dstLight->_position = srcLight->GetPosition();
			dstLight->_constantAttenuation = srcLight->GetConstantAttenuation();
			dstLight->_linearAttenuation = srcLight->GetLinearAttenuation();
			dstLight->_quadraticAttenuation = srcLight->GetQuadraticAttenuation();
		}

		void Renderer::UpdateSpotLight(LightingUniform::FragmentSpotLight* dstLight, const boost::shared_ptr<Entities::SpotLight>& srcLight)
		{
			UpdatePointLight(&dstLight->_base, srcLight);
			dstLight->_direction = srcLight->GetDirection();
			dstLight->_cutoffAngle = cosf(Math::ToRadians(srcLight->GetCutoffAngle()));
		}

		void Renderer::UpdateCameraUniformBuffers()
		{
			for (usize i = 0; i < _cameraUniformBuffersMemory.size(); ++i)
			{
				// Map pointer to buffer location.
				void* data;
				vkMapMemory(_device._logical, _cameraUniformBuffersMemory[i], 0, sizeof(CameraUniform), 0, &data);

				// Transfer data to buffer memory.
				Memory::Copy(data, &_vertexUniform, sizeof(CameraUniform));

				// Unmap memory from pointer.
				vkUnmapMemory(_device._logical, _cameraUniformBuffersMemory[i]);
			}
		}

		void Renderer::UpdateLightingUniformBuffers()
		{
			for (usize i = 0; i < _lightingUniformBuffersMemory.size(); ++i)
			{
				// Map pointer to buffer location.
				void* data;
				vkMapMemory(_device._logical, _lightingUniformBuffersMemory[i], 0, sizeof(LightingUniform), 0, &data);

				// Transfer data to buffer memory.
				Memory::Copy(data, &_fragmentUniform, sizeof(LightingUniform));

				// Unmap memory from pointer.
				vkUnmapMemory(_device._logical, _lightingUniformBuffersMemory[i]);
			}
		}

		void Renderer::UpdateMaterialUniformBuffers()
		{
			const usize uniformAlignment = GetAlignedSize(sizeof(MaterialUniform), _minUniformBufferAlignment);
			for (usize i = 0; i < _materialUniformBuffersMemory.size(); ++i)
			{
				// Map pointer to buffer location.
				void* data;
				vkMapMemory(_device._logical, _materialUniformBuffersMemory[i], 0, uniformAlignment *
					_entitiesToRender.size() + _entitiesToTransfer.size(), 0, &data);

				// Copy entity data over to fragment dynamic buffer.
				usize j = 0;
				BOOST_FOREACH(const auto& entity, boost::join(_entitiesToRender, _entitiesToTransfer))
				{
					for (auto& renderableInfo : entity.second._renderables)
					{
						// Construct data to place into shader.
						MaterialUniform material = {};
						material._specularPower = renderableInfo._material->GetSpecularPower();
						material._specularStrength = renderableInfo._material->GetSpecularStrength();

						// Read memory with specified alignment.
						MaterialUniform* dynamicMemory = (MaterialUniform*)((usize)data + (j * uniformAlignment));
						*dynamicMemory = material;

						// Move index over to next dynamic uniform location.
						j++;
					}
				}

				// Unmap memory from pointer.
				vkUnmapMemory(_device._logical, _materialUniformBuffersMemory[i]);
			}
		}
		
		void Renderer::DestroyCommandPools()
		{
			vkDestroyCommandPool(_device._logical, _graphicsPool, nullptr);
			vkDestroyCommandPool(_device._logical, _transferPool, nullptr);
		}

		void Renderer::DestroyEntities()
		{
			// Destroy each of the existing rendarable entities.
			for (auto& entity : _entitiesToRender)
			{
				for (auto& renderableInfo : entity.second._renderables)
				{
					DestroyBuffer(renderableInfo._vertexBuffer);
					DestroyBuffer(renderableInfo._indexBuffer);
					DestroyImage(renderableInfo._diffuseImage);
				}
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

		void Renderer::DestroyDescriptorSetLayouts()
		{
			// Destroy layouts associated with the lighting pipeline.
			vkDestroyDescriptorSetLayout(_device._logical, _gBufferDescriptorSetLayout, nullptr);
			vkDestroyDescriptorSetLayout(_device._logical, _lightingDescriptorSetLayout, nullptr);

			// Destroy layouts associated with the G-buffer pipeline.
			vkDestroyDescriptorSetLayout(_device._logical, _samplerDescriptorSetLayout, nullptr);
			vkDestroyDescriptorSetLayout(_device._logical, _bufferDescriptorSetLayout, nullptr);
		}

		void Renderer::DestroyGraphicsPipelines()
		{
			// Destroy the G-buffer graphics pipeline.
			vkDestroyPipeline(_device._logical, _gBufferPipeline, nullptr);
			vkDestroyPipelineLayout(_device._logical, _gBufferPipelineLayout, nullptr);

			// Destroy the lighting graphics pipeline.
			vkDestroyPipeline(_device._logical, _lightingPipeline, nullptr);
			vkDestroyPipelineLayout(_device._logical, _lightingPipelineLayout, nullptr);
		}

		void Renderer::DestroyGBuffer()
		{
			// Destroy the color buffer.
			for (usize i = 0; i < _colorBuffer.size(); ++i)
			{
				vkDestroyImageView(_device._logical, _colorBuffer[i]._view, nullptr);
				vkDestroyImage(_device._logical, _colorBuffer[i]._raw, nullptr);
				vkFreeMemory(_device._logical, _colorBuffer[i]._memory, nullptr);
			}

			// Destroy the depth buffer.
			for (usize i = 0; i < _depthBuffer.size(); ++i)
			{
				vkDestroyImageView(_device._logical, _depthBuffer[i]._view, nullptr);
				vkDestroyImage(_device._logical, _depthBuffer[i]._raw, nullptr);
				vkFreeMemory(_device._logical, _depthBuffer[i]._memory, nullptr);
			}

			// Destroy the normal buffer.
			for (usize i = 0; i < _normalBuffer.size(); ++i)
			{
				vkDestroyImageView(_device._logical, _normalBuffer[i]._view, nullptr);
				vkDestroyImage(_device._logical, _normalBuffer[i]._raw, nullptr);
				vkFreeMemory(_device._logical, _normalBuffer[i]._memory, nullptr);
			}

			// Destroy the position buffer.
			for (usize i = 0; i < _positionBuffer.size(); ++i)
			{
				vkDestroyImageView(_device._logical, _positionBuffer[i]._view, nullptr);
				vkDestroyImage(_device._logical, _positionBuffer[i]._raw, nullptr);
				vkFreeMemory(_device._logical, _positionBuffer[i]._memory, nullptr);
			}
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
			// Destroy lighting uniform buffers.
			for (usize i = 0; i < _lightingUniformBuffers.size(); ++i)
			{
				vkDestroyBuffer(_device._logical, _lightingUniformBuffers[i], nullptr);
				vkFreeMemory(_device._logical, _lightingUniformBuffersMemory[i], nullptr);
			}

			// Destroy material uniform buffers.
			for (usize i = 0; i < _materialUniformBuffers.size(); ++i)
			{
				vkDestroyBuffer(_device._logical, _materialUniformBuffers[i], nullptr);
				vkFreeMemory(_device._logical, _materialUniformBuffersMemory[i], nullptr);
			}

			// Destroy camera uniform buffers.
			for (usize i = 0; i < _cameraUniformBuffers.size(); ++i)
			{
				vkDestroyBuffer(_device._logical, _cameraUniformBuffers[i], nullptr);
				vkFreeMemory(_device._logical, _cameraUniformBuffersMemory[i], nullptr);
			}
		}

		void Renderer::DestroyDescriptorPools()
		{
			// Destroy descriptor pools associated with the lighting pipeline.
			vkDestroyDescriptorPool(_device._logical, _gBufferDescriptorPool, nullptr);
			vkDestroyDescriptorPool(_device._logical, _lightingDescriptorPool, nullptr);

			// Destroy descriptor pools associated with the g-buffer pipeline.
			vkDestroyDescriptorPool(_device._logical, _samplerDescriptorPool, nullptr);
			vkDestroyDescriptorPool(_device._logical, _bufferDescriptorPool, nullptr);
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
