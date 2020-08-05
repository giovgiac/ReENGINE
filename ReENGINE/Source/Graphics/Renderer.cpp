/*
 * Renderer.cpp
 *
 * Copyright (c) Giovanni Giacomo. All Rights Reserved.
 *
 */

#include "Renderer.hpp"

#include <boost/container/set.hpp>
#include <fstream>

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
			// Store window handle
			_window = &window;

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
			CHECK_RESULT_WITH_ERROR(CreateSwapchain(), RendererResult::Success, NTEXT("Failed to create swapchain!\n"), RendererResult::Failure);
			CHECK_RESULT_WITH_ERROR(CreateRenderPass(), RendererResult::Success, NTEXT("Failed to create renderpass!\n"), RendererResult::Failure);
			CHECK_RESULT_WITH_ERROR(CreateGraphicsPipeline(), RendererResult::Success, NTEXT("Failed to create graphics pipeline!\n"), RendererResult::Failure);

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
			vkDestroyPipeline(_device._logical, _graphicsPipeline, nullptr);
			vkDestroyPipelineLayout(_device._logical, _pipelineLayout, nullptr);
			vkDestroyRenderPass(_device._logical, _renderPass, nullptr);
			DestroySwapchain();
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

		RendererResult Renderer::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags flags, VkImageView* outView) const
		{
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

			if (outView)
			{
				CHECK_RESULT(vkCreateImageView(_device._logical, &createInfo, nullptr, outView), VK_SUCCESS, RendererResult::Failure);
				return RendererResult::Success;
			}

			return RendererResult::Failure;
		}

		RendererResult Renderer::CreateShaderModule(const boost::container::vector<char>& raw, VkShaderModule* outModule) const
		{
			VkShaderModuleCreateInfo createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			createInfo.codeSize = raw.size();
			createInfo.pCode = reinterpret_cast<const u32*>(raw.data());

			if (outModule)
			{
				CHECK_RESULT(vkCreateShaderModule(_device._logical, &createInfo, nullptr, outModule), VK_SUCCESS, RendererResult::Failure);
				return RendererResult::Success;
			}

			return RendererResult::Failure;
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
			
			CHECK_RESULT(vkCreateSwapchainKHR(_device._logical, &createInfo, nullptr, &_swapchain), VK_SUCCESS, RendererResult::Failure);

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
				CHECK_RESULT(CreateImageView(img, _swapchainFormat, VK_IMAGE_ASPECT_COLOR_BIT, &swapchainImage._view), RendererResult::Success, RendererResult::Failure);

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

			// Attachment reference for subpass.
			VkAttachmentReference colorReference = {};
			colorReference.attachment = 0;
			colorReference.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			// Subpass description for this render pass.
			VkSubpassDescription subpass = {};
			subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
			subpass.colorAttachmentCount = 1;
			subpass.pColorAttachments = &colorReference;

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

			// Render pass creation information.
			VkRenderPassCreateInfo renderCreateInfo = {};
			renderCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			renderCreateInfo.attachmentCount = 1;
			renderCreateInfo.pAttachments = &colorAttachment;
			renderCreateInfo.subpassCount = 1;
			renderCreateInfo.pSubpasses = &subpass;
			renderCreateInfo.dependencyCount = static_cast<u32>(subpassDependencies.size());
			renderCreateInfo.pDependencies = subpassDependencies.data();
			
			CHECK_RESULT(vkCreateRenderPass(_device._logical, &renderCreateInfo, nullptr, &_renderPass), VK_SUCCESS, RendererResult::Failure);
			return RendererResult::Success;
		}

		RendererResult Renderer::CreateGraphicsPipeline()
		{
			boost::container::vector<char> fragmentShaderRaw, vertexShaderRaw;
			CHECK_RESULT(ReadShader("Shaders/vert.spv", &vertexShaderRaw), RendererResult::Success, RendererResult::Failure);
			CHECK_RESULT(ReadShader("Shaders/frag.spv", &fragmentShaderRaw), RendererResult::Success, RendererResult::Failure);

			// Create shader modules.
			VkShaderModule fragmentShader, vertexShader;
			CHECK_RESULT(CreateShaderModule(vertexShaderRaw, &vertexShader), RendererResult::Success, RendererResult::Failure);
			CHECK_RESULT(CreateShaderModule(fragmentShaderRaw, &fragmentShader), RendererResult::Success, RendererResult::Failure);

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

			// Vertex input stage.
			VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
			vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
			vertexInputCreateInfo.pVertexBindingDescriptions = nullptr;
			vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
			vertexInputCreateInfo.pVertexAttributeDescriptions = nullptr;

			#pragma endregion This stage is responsible for configuring the data that goes into the graphics pipeline.
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
			rasterCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
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
			layoutCreateInfo.setLayoutCount = 0;
			layoutCreateInfo.pSetLayouts = nullptr;
			layoutCreateInfo.pushConstantRangeCount = 0;
			layoutCreateInfo.pPushConstantRanges = nullptr;

			// Create pipeline layout.
			CHECK_RESULT(vkCreatePipelineLayout(_device._logical, &layoutCreateInfo, nullptr, &_pipelineLayout), VK_SUCCESS, RendererResult::Failure);
			
			#pragma endregion

			// TODO: Setup depth stencil testing

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
			pipelineCreateInfo.pDepthStencilState = nullptr;
			pipelineCreateInfo.layout = _pipelineLayout;
			pipelineCreateInfo.renderPass = _renderPass;							// Render pass it is compatible with.
			pipelineCreateInfo.subpass = 0;											// Subpass of the render pass that it will use.

			// Derivative graphics pipelines (use these attributes to base a pipeline onto another).
			pipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE;
			pipelineCreateInfo.basePipelineIndex = -1;

			CHECK_RESULT(vkCreateGraphicsPipelines(_device._logical, VK_NULL_HANDLE, 1, &pipelineCreateInfo, nullptr, &_graphicsPipeline), VK_SUCCESS, RendererResult::Failure);
			
			// Destroy shader modules (no longer needed after creating pipeline).
			vkDestroyShaderModule(_device._logical, fragmentShader, nullptr);
			vkDestroyShaderModule(_device._logical, vertexShader, nullptr);

			return RendererResult::Success;
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
