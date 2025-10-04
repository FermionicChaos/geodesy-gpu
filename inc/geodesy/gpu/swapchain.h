#ifndef GEODESY_GPU_SWAPCHAIN_H
#define GEODESY_GPU_SWAPCHAIN_H

#include "config.h"

#include "framechain.h"

namespace geodesy::gpu {

	class swapchain : public framechain{
	public:

		VkSwapchainCreateInfoKHR 		CreateInfo;
		VkSwapchainKHR 					Handle;

		swapchain();
		swapchain(std::shared_ptr<context> aContext, VkSurfaceKHR aSurface);
		~swapchain();
		
	};

}

#endif // !GEODESY_GPU_SWAPCHAIN_H
