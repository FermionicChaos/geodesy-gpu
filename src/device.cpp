#include <geodesy/gpu/device.h>
#include <algorithm>

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

	std::vector<int> device::sort_queue_family_indices(unsigned int aDesiredOperations) const {
		std::vector<std::pair<size_t, int>> candidates; // <operation_count, family_index>
		
		// Collect compatible queue families with their operation counts
		for (size_t i = 0; i < QueueFamilyProperties.size(); i++) {
			const VkQueueFlags queue_flags = QueueFamilyProperties[i].queueFlags;
			
			// Check if this family supports all desired operations
			if ((queue_flags & aDesiredOperations) == aDesiredOperations) {
				// Count total operations supported by this queue family
				size_t operation_count = 0;
				for (unsigned int op = 1; op != 0; op <<= 1) {
					if (queue_flags & op) operation_count++;
				}
				
				candidates.push_back(std::make_pair(operation_count, static_cast<int>(i)));
			}
		}
		
		// Sort by operation count (ascending - most specific first)
		std::sort(candidates.begin(), candidates.end());
		
		// Extract sorted indices
		std::vector<int> sorted_indices;
		sorted_indices.reserve(candidates.size());
		for (const auto& candidate : candidates) {
			sorted_indices.push_back(candidate.second);
		}
		
		return sorted_indices;
	}

}
