#ifndef GEODESY_GPU_DEVICE_H
#define GEODESY_GPU_DEVICE_H

#include "config.h"

namespace geodesy::gpu {

	class device  {
	public:

		// Device Memory Types
		enum memory {
			DEVICE_LOCAL 				= VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			HOST_VISIBLE 				= VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
			HOST_COHERENT 				= VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			HOST_CACHED 				= VK_MEMORY_PROPERTY_HOST_CACHED_BIT,
			LAZILY_ALLOCATED 			= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT,
			// PROTECTED 					= VK_MEMORY_PROPERTY_PROTECTED_BIT,
			DEVICE_COHERENT 			= VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD,
			DEVICE_UNCACHED 			= VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD,
			RDMA_CAPABLE 				= VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV,
		};

		// Device Executable Operations
		enum operation {
			GRAPHICS 					= VK_QUEUE_GRAPHICS_BIT,
			COMPUTE 					= VK_QUEUE_COMPUTE_BIT,
			TRANSFER 					= VK_QUEUE_TRANSFER_BIT,
			SPARSE_BINDING 				= VK_QUEUE_SPARSE_BINDING_BIT,
			// PROTECTED 					= VK_QUEUE_PROTECTED_BIT,
			VIDEO_DECODE 				= VK_QUEUE_VIDEO_DECODE_BIT_KHR,
			VIDEO_ENCODE 				= VK_QUEUE_VIDEO_ENCODE_BIT_KHR,
			OPTICAL_FLOW 				= VK_QUEUE_OPTICAL_FLOW_BIT_NV,
		};

		std::string 								Name;
		std::string 								Type;

		VkPhysicalDeviceProperties 					Properties;
		VkPhysicalDeviceMemoryProperties 			MemoryProperties;
		std::vector<VkQueueFamilyProperties> 		QueueFamilyProperties;
		VkPhysicalDeviceFeatures 					Features;
		VkPhysicalDevice 							Handle;

		device();
		device(instance* aInstance, VkPhysicalDevice aPhysicalDevice);
		~device();

	};

}

#endif // !GEODESY_GPU_DEVICE_H