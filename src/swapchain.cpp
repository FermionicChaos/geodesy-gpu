#include <geodesy/gpu/swapchain.h>

#include <algorithm>

#include <geodesy/gpu/context.h>
#include <geodesy/gpu/instance.h>

namespace geodesy::gpu {

	swapchain::create_info::create_info() {
		this->FrameCount		= 3;
		this->FrameRate			= 60.0f;
		this->PixelFormat		= image::format::B8G8R8A8_UNORM; // HDR is image::format::R16G16B16A16_SFLOAT
		this->ColorSpace		= swapchain::colorspace::SRGB_NONLINEAR;
		this->ImageUsage		= image::usage::COLOR_ATTACHMENT;
		this->CompositeAlpha	= swapchain::composite::ALPHA_OPAQUE;
		this->PresentMode		= swapchain::present_mode::FIFO;
		this->Clipped			= false;
	}

	swapchain::create_info::create_info(VkSwapchainCreateInfoKHR aCreateInfo, float aFrameRate) {
		this->FrameCount 		= aCreateInfo.minImageCount;
		this->FrameRate 		= aFrameRate;
		this->PixelFormat 		= (image::format)aCreateInfo.imageFormat;
		this->ColorSpace 		= (swapchain::colorspace)aCreateInfo.imageColorSpace;
		this->ImageUsage 		= (image::usage)aCreateInfo.imageUsage;
		this->CompositeAlpha 	= (swapchain::composite)aCreateInfo.compositeAlpha;
		this->PresentMode 		= (swapchain::present_mode)aCreateInfo.presentMode;
		this->Clipped 			= aCreateInfo.clipped;		
	}

	swapchain::swapchain() : framechain() {
		this->Surface = VK_NULL_HANDLE;
		this->CreateInfo = {};
		this->Handle = VK_NULL_HANDLE;
		this->PresentationQueue = VK_NULL_HANDLE;
		this->AcquirePresentFrameSemaphore = std::make_pair(nullptr, nullptr);
	}

	swapchain::swapchain(std::shared_ptr<context> aContext, VkSurfaceKHR aSurface, const create_info& aCreateInfo, VkSwapchainKHR aOldSwapchain) : framechain() {
		VkResult Result = VK_SUCCESS;
		// Load physical device functions from instance (they operate on VkPhysicalDevice)
		PFN_vkGetPhysicalDeviceSurfaceSupportKHR vkGetPhysicalDeviceSurfaceSupportKHR = (PFN_vkGetPhysicalDeviceSurfaceSupportKHR)aContext->Instance->function_pointer("vkGetPhysicalDeviceSurfaceSupportKHR");
		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)aContext->Instance->function_pointer("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
		// Load device functions from device context (they operate on VkDevice)
		PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)aContext->function_pointer("vkCreateSwapchainKHR");
		PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)aContext->function_pointer("vkGetSwapchainImagesKHR");

		// Find Present Queue in aContext	
		VkBool32 SupportsPresent = VK_FALSE;
		for (auto& [Op, Q] : aContext->Queue) {
			// Test if this queue supports present.
			vkGetPhysicalDeviceSurfaceSupportKHR(aContext->Device->Handle, Q.FamilyIndex, aSurface, &SupportsPresent);
			if (SupportsPresent == VK_TRUE) {
				this->PresentationQueue = Q.Handle;
				break;
			}
		}

		if (SupportsPresent == VK_FALSE) {
			throw std::runtime_error("Failed to find a queue that supports presentation to the given surface.");
		}

		VkSurfaceCapabilitiesKHR SurfaceCapabilities;
		Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(aContext->Device->Handle, aSurface, &SurfaceCapabilities);
		if (Result != VK_SUCCESS) {
			throw std::runtime_error("Failed to get surface capabilities.");
		}
		
		this->Context                             	= aContext;
		this->Surface 								= aSurface;

		// Fill out create info.
		this->CreateInfo.sType						= VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		this->CreateInfo.pNext						= NULL;
		this->CreateInfo.flags						= 0;
		this->CreateInfo.surface					= aSurface;
		this->CreateInfo.minImageCount				= aCreateInfo.FrameCount;
		this->CreateInfo.imageFormat				= (VkFormat)aCreateInfo.PixelFormat;
		this->CreateInfo.imageColorSpace			= (VkColorSpaceKHR)aCreateInfo.ColorSpace;
		this->CreateInfo.imageExtent				= SurfaceCapabilities.currentExtent;
		this->CreateInfo.imageArrayLayers			= 1;
		this->CreateInfo.imageUsage					= aCreateInfo.ImageUsage;
		this->CreateInfo.imageSharingMode			= VK_SHARING_MODE_EXCLUSIVE;
		this->CreateInfo.queueFamilyIndexCount		= 0;
		this->CreateInfo.pQueueFamilyIndices		= NULL;
		this->CreateInfo.preTransform				= SurfaceCapabilities.currentTransform;
		this->CreateInfo.compositeAlpha				= (VkCompositeAlphaFlagBitsKHR)aCreateInfo.CompositeAlpha;
		this->CreateInfo.presentMode				= (VkPresentModeKHR)aCreateInfo.PresentMode;
		this->CreateInfo.clipped					= aCreateInfo.Clipped;
		this->CreateInfo.oldSwapchain				= aOldSwapchain;

		this->Resolution = { CreateInfo.imageExtent.width, CreateInfo.imageExtent.height, 1u };

		Result = vkCreateSwapchainKHR(aContext->Handle, &CreateInfo, NULL, &this->Handle);
		if (Result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create swapchain.");
		}
		else {
			// Get swapchain images.
			uint32_t ImageCount = 0;
			Result = vkGetSwapchainImagesKHR(aContext->Handle, this->Handle, &ImageCount, NULL);
			std::vector<VkImage> ImageList(ImageCount);
			Result = vkGetSwapchainImagesKHR(aContext->Handle, this->Handle, &ImageCount, ImageList.data());
			this->Image = std::vector<std::map<std::string, std::shared_ptr<image>>>(ImageCount);
			for (std::size_t i = 0; i < ImageList.size(); i++) {
				this->Image[i]["Color"] = geodesy::make<image>();
				this->Image[i]["Color"]->Context = aContext;
				this->Image[i]["Color"]->CreateInfo = this->image_create_info();
				this->Image[i]["Color"]->Handle = ImageList[i];
				this->Image[i]["Color"]->transition(image::layout::LAYOUT_UNDEFINED, image::layout::PRESENT_SRC_KHR);
				this->Image[i]["Color"]->View = this->Image[i]["Color"]->view();
			}
			// Create Semaphores for acquiring and rendering frames.
			for (std::size_t i = 0; i < ImageList.size(); i++) {
				this->SemaphoreQueue.push(std::make_pair(aContext->create<semaphore>(), aContext->create<semaphore>()));
			}
		}

	}

	swapchain::~swapchain() {
		PFN_vkDestroySwapchainKHR vkDestroySwapchainKHR = (PFN_vkDestroySwapchainKHR)Context->function_pointer("vkDestroySwapchainKHR");
		PFN_vkDestroyImageView vkDestroyImageView = (PFN_vkDestroyImageView)Context->function_pointer("vkDestroyImageView");
		// Null out images so they don't try to free themselves.
		for (auto& I : this->Image) {
			// Per attachment
			for (auto& [Name, Img] : I) {
				Img->Handle = VK_NULL_HANDLE;
				// TODO: Destroy this
				vkDestroyImageView(Context->Handle, Img->View, NULL);
				Img->View = VK_NULL_HANDLE;
			}
		}
		vkDestroySwapchainKHR(Context->Handle, Handle, NULL);
	}

	VkImageCreateInfo swapchain::image_create_info() const {
		VkImageCreateInfo ImageCreateInfo{};
		ImageCreateInfo.sType					= VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
		ImageCreateInfo.pNext					= NULL;
		ImageCreateInfo.flags					= 0;
		ImageCreateInfo.imageType				= VK_IMAGE_TYPE_2D;
		ImageCreateInfo.format					= CreateInfo.imageFormat;
		ImageCreateInfo.extent					= { CreateInfo.imageExtent.width, CreateInfo.imageExtent.height, 1u };
		ImageCreateInfo.mipLevels				= 1;
		ImageCreateInfo.arrayLayers				= CreateInfo.imageArrayLayers;
		ImageCreateInfo.samples					= (VkSampleCountFlagBits)image::sample::COUNT_1;
		ImageCreateInfo.tiling					= (VkImageTiling)image::tiling::OPTIMAL;
		ImageCreateInfo.usage					= CreateInfo.imageUsage;
		ImageCreateInfo.sharingMode				= CreateInfo.imageSharingMode;
		ImageCreateInfo.queueFamilyIndexCount	= 0;
		ImageCreateInfo.pQueueFamilyIndices		= NULL;
		ImageCreateInfo.initialLayout			= (VkImageLayout)image::LAYOUT_UNDEFINED;
		return ImageCreateInfo;
	}

	VkResult swapchain::next_frame() {
		VkResult Result = VK_SUCCESS;
		PFN_vkQueuePresentKHR vkQueuePresentKHR = (PFN_vkQueuePresentKHR)Context->function_pointer("vkQueuePresentKHR");
		// PFN_vkQueueWaitIdle vkQueueWaitIdle = (PFN_vkQueueWaitIdle)Context->function_pointer("vkQueueWaitIdle");
		PFN_vkAcquireNextImageKHR vkAcquireNextImageKHR = (PFN_vkAcquireNextImageKHR)Context->function_pointer("vkAcquireNextImageKHR");

		// Present previous frame if we have a valid present semaphore
		if (this->AcquirePresentFrameSemaphore.second != nullptr) {
			VkPresentInfoKHR PresentInfo{};
			PresentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
			PresentInfo.pNext				= NULL;
			PresentInfo.waitSemaphoreCount	= 1;
			PresentInfo.pWaitSemaphores		= &this->AcquirePresentFrameSemaphore.second->Handle;
			PresentInfo.swapchainCount		= 1;
			PresentInfo.pSwapchains			= &this->Handle;
			PresentInfo.pImageIndices		= &this->DrawIndex;
			
			// Present current image to screen
			Result = vkQueuePresentKHR(this->PresentationQueue, &PresentInfo);
			if (Result != VK_SUCCESS) {
				this->SemaphoreQueue.push(this->AcquirePresentFrameSemaphore);
				return Result;
			}

			// // Ensure presentation is complete before reusing semaphores
			// Result = vkQueueWaitIdle(this->PresentationQueue);
			// if (Result != VK_SUCCESS) {
			// 	// Handle wait errors
			// 	return Result;
			// }
			
			// Return used semaphore pair to queue for reuse 
			// NOTE: This assumes the present operation has completed when we call next_frame again
			// In a real implementation, you might want to use fences to track completion
			this->SemaphoreQueue.push(this->AcquirePresentFrameSemaphore);
		}

		// // Get next semaphore pair from queue
		// if (this->SemaphoreQueue.empty()) {
		// 	return VK_NOT_READY; // No available semaphores, should not happen if used correctly
		// }
		
		// Acquire new semaphore pair
		this->AcquirePresentFrameSemaphore = this->SemaphoreQueue.front();
		this->SemaphoreQueue.pop();

		// Set previous draw index for read operations
		this->ReadIndex = this->DrawIndex;

		// Acquire next swapchain image
		Result = vkAcquireNextImageKHR(Context->Handle, this->Handle, UINT64_MAX, this->AcquirePresentFrameSemaphore.first->Handle, VK_NULL_HANDLE, &this->DrawIndex);
		if (Result != VK_SUCCESS) {
			// Return semaphore, acquire failed to get next image.
			this->SemaphoreQueue.push(this->AcquirePresentFrameSemaphore);
			return Result;
		}

		// Return semaphore pair:
		// .first  = image acquisition semaphore (signaled when image is ready for rendering)
		// .second = render completion semaphore (signal this when rendering is complete)
		return Result;
	}

	std::pair<std::shared_ptr<semaphore>, std::shared_ptr<semaphore>> swapchain::get_acquire_present_semaphore_pair() {
		return this->AcquirePresentFrameSemaphore;
	}

}
