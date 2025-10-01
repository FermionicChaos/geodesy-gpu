#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	context::context() {

	}

	context::context(
		std::shared_ptr<instance> 	aInstance,
		std::shared_ptr<device> 	aDevice,
		std::vector<int> 			aExecutionOperations,
		std::set<std::string> 		aLayers,
		std::set<std::string> 		aExtensions,
		void* 						aNext
	) : context() {
		VkResult Result = VK_SUCCESS;
		this->Instance = aInstance;
		this->Device = aDevice;

		std::vector<const char*> LayerList;
		for (const auto& Layer : aLayers) {
			LayerList.push_back(Layer.c_str());
		}

		std::vector<const char*> ExtensionList;
		for (const auto& Extension : aExtensions) {
			ExtensionList.push_back(Extension.c_str());
		}

		std::vector<std::vector<float>> QueuePriority;
		std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo;

		// Write code here...

		// Fill out structures here.
		for (size_t i = 0; i < QueueCreateInfo.size(); i++) {
			QueueCreateInfo[i].sType					= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfo[i].pNext					= NULL;
			QueueCreateInfo[i].flags					= 0;
			QueueCreateInfo[i].queueFamilyIndex			;
			QueueCreateInfo[i].queueCount				= QueuePriority[i].size();
			QueueCreateInfo[i].pQueuePriorities			= QueuePriority[i].data();
		}

		VkDeviceCreateInfo DCI = {};
		DCI.sType							= VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
		DCI.pNext							= aNext;
		DCI.flags							= 0;
		DCI.queueCreateInfoCount			= QueueCreateInfo.size();
		DCI.pQueueCreateInfos				= QueueCreateInfo.data();
		DCI.enabledLayerCount				= LayerList.size();
		DCI.ppEnabledLayerNames				= LayerList.data();
		DCI.enabledExtensionCount			= ExtensionList.size();
		DCI.ppEnabledExtensionNames			= ExtensionList.data();
		DCI.pEnabledFeatures				= &aDevice->Features;

		Result = vkCreateDevice(Device->Handle, &DCI, NULL, &this->Handle);
		if (Result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create context.");
		}
	}

	context::~context() {

	}

}
