#include <geodesy/gpu/swapchain.h>

#include <algorithm>

#include <geodesy/gpu/context.h>

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

	swapchain::swapchain(std::shared_ptr<context> aContext, VkSurfaceKHR aSurface, const create_info& aCreateInfo, VkSwapchainKHR aOldSwapchain) {
		VkResult Result = VK_SUCCESS;

		PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR vkGetPhysicalDeviceSurfaceCapabilitiesKHR = (PFN_vkGetPhysicalDeviceSurfaceCapabilitiesKHR)aContext->function_pointer("vkGetPhysicalDeviceSurfaceCapabilitiesKHR");
		PFN_vkCreateSwapchainKHR vkCreateSwapchainKHR = (PFN_vkCreateSwapchainKHR)aContext->function_pointer("vkCreateSwapchainKHR");
		PFN_vkGetSwapchainImagesKHR vkGetSwapchainImagesKHR = (PFN_vkGetSwapchainImagesKHR)aContext->function_pointer("vkGetSwapchainImagesKHR");

		VkSurfaceCapabilitiesKHR SurfaceCapabilities;
		Result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(aContext->Device->Handle, aSurface, &SurfaceCapabilities);
		
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
			for (std::size_t i = 0; i < ImageList.size(); i++) {
				this->Image[i]["Color"] = geodesy::make<image>();
				this->Image[i]["Color"]->Context = aContext;
				this->Image[i]["Color"]->CreateInfo = this->image_create_info();
				this->Image[i]["Color"]->Handle = ImageList[i];
				this->Image[i]["Color"]->transition(image::layout::LAYOUT_UNDEFINED, image::layout::PRESENT_SRC_KHR);
				this->Image[i]["Color"]->View = this->Image[i]["Color"]->view();
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

	VkResult swapchain::next_frame(VkSemaphore aPresentFrameSemaphore, VkSemaphore aNextFrameSemaphore, VkFence aNextFrameFence) {
		VkResult ReturnValue = VK_SUCCESS;

		// // Before acquiring next image, present current image.
		// if (aPresentFrameSemaphore != VK_NULL_HANDLE){
		// 	VkPresentInfoKHR PresentInfo{};
		// 	PresentInfo.sType				= VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		// 	PresentInfo.pNext				= NULL;
		// 	PresentInfo.waitSemaphoreCount	= 1;
		// 	PresentInfo.pWaitSemaphores		= &aPresentFrameSemaphore;
		// 	PresentInfo.swapchainCount		= 1;
		// 	PresentInfo.pSwapchains			= &Handle;
		// 	PresentInfo.pImageIndices		= &DrawIndex;
		// 	// Present image to screen.
		// 	ReturnValue = vkQueuePresentKHR(Context->Queue[device::operation::PRESENT], &PresentInfo);
		// }

		// Set previous draw index for read operations.
		ReadIndex = DrawIndex;

		// Acquire new image.
		return vkAcquireNextImageKHR(Context->Handle, Handle, UINT64_MAX, aNextFrameSemaphore, aNextFrameFence, &DrawIndex);
	}

}
