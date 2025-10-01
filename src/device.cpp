#include <geodesy/gpu/device.h>

namespace geodesy::gpu {

	device::device() {
		this->Name = "";
		this->Type = "";

		this->Handle = VK_NULL_HANDLE;
		this->Properties = {};
		this->MemoryProperties = {};
		this->QueueFamilyProperties = {};
		this->Features = {};
	}
	
	device::device(instance* aInstance, VkPhysicalDevice aPhysicalDevice) {
		this->Handle = aPhysicalDevice;

		// Query Device Properties
		vkGetPhysicalDeviceProperties(this->Handle, &this->Properties);

		// Query Device Memory Properties
		vkGetPhysicalDeviceMemoryProperties(this->Handle, &this->MemoryProperties);

		// Query Queue Family Properties
		uint32_t QueueFamilyPropertyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(this->Handle, &QueueFamilyPropertyCount, NULL);
		this->QueueFamilyProperties = std::vector<VkQueueFamilyProperties>(QueueFamilyPropertyCount);
		vkGetPhysicalDeviceQueueFamilyProperties(this->Handle, &QueueFamilyPropertyCount, this->QueueFamilyProperties.data());

		// Query Device Features
		vkGetPhysicalDeviceFeatures(this->Handle, &this->Features);

		this->Name = std::string(this->Properties.deviceName);

		switch (this->Properties.deviceType) {
			case VK_PHYSICAL_DEVICE_TYPE_OTHER:
				this->Type = "Other";
				break;
			case VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU:
				this->Type = "Integrated GPU";
				break;
			case VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU:
				this->Type = "Discrete GPU";
				break;
			case VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU:
				this->Type = "Virtual GPU";
				break;
			case VK_PHYSICAL_DEVICE_TYPE_CPU:
				this->Type = "CPU";
				break;
			default:
				this->Type = "Unknown";
				break;
		}
	}
	
	device::~device() {}	

}
