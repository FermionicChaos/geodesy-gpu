#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	context::context() {

	}

	context::context(
		std::shared_ptr<instance> 		aInstance,
		std::shared_ptr<device> 		aDevice,
		std::vector<unsigned int> 		aExecutionOperations,
		std::set<std::string> 			aLayers,
		std::set<std::string> 			aExtensions,
		void* 							aNext
	) : context() {
		VkResult Result = VK_SUCCESS;
		this->Instance = aInstance;
		this->Device = aDevice;

		// This keeps track of how many queues have been used up in QueueIndexMap.
		std::vector<int> QueueOffset(aDevice->QueueFamilyProperties.size(), 0);

		// This builds the index map <qfi, qi> for each requested operation.
		for (auto& Operation : aExecutionOperations) {
			// Get list of sorted indices starting with qfis most similar to desired operations.
			std::vector<int> SortedQueueFamilyIndices = aDevice->sort_queue_family_indices(Operation);

			// For each ordered queue family index, check if there is a free queue index to use in family.
			for (size_t i = 0; i < SortedQueueFamilyIndices.size(); i++) {
				if (QueueOffset[SortedQueueFamilyIndices[i]] < aDevice->QueueFamilyProperties[SortedQueueFamilyIndices[i]].queueCount) {
					// Found a queue family index with a free queue index.
					IndexMap[Operation] = std::make_pair(SortedQueueFamilyIndices[i], QueueOffset[SortedQueueFamilyIndices[i]]);
					// Offset into Queue Family.
					QueueOffset[SortedQueueFamilyIndices[i]]++;
					break;
				}
			}
		}

		// Create Structures for device creation from IndexMap.
		std::map<int, int> ReducedIndexMap; // <qfi, queue_count>
		for (const auto& [Op, ij] : IndexMap) {
			// For each qfi in IndexMap, count how many times it appears.
			ReducedIndexMap[ij.first]++;
		}

		// Linearize the reduced index map.
		std::vector<std::pair<int, int>> LinearizedIndexMap(ReducedIndexMap.begin(), ReducedIndexMap.end());

		// Fill out queue priorities and create info structures.
		std::vector<std::vector<float>> QueuePriority(LinearizedIndexMap.size());
		std::vector<VkDeviceQueueCreateInfo> QueueCreateInfo(LinearizedIndexMap.size());
		for (size_t i = 0; i < QueueCreateInfo.size(); i++) {
			QueuePriority[i] 							= std::vector<float>(LinearizedIndexMap[i].second, 1.0f);
			QueueCreateInfo[i].sType					= VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
			QueueCreateInfo[i].pNext					= NULL;
			QueueCreateInfo[i].flags					= 0;
			QueueCreateInfo[i].queueFamilyIndex			= LinearizedIndexMap[i].first;
			QueueCreateInfo[i].queueCount				= LinearizedIndexMap[i].second;
			QueueCreateInfo[i].pQueuePriorities			= QueuePriority[i].data();
		}

		std::vector<const char*> LayerList;
		for (const auto& Layer : aLayers) {
			LayerList.push_back(Layer.c_str());
		}

		std::vector<const char*> ExtensionList;
		for (const auto& Extension : aExtensions) {
			ExtensionList.push_back(Extension.c_str());
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

	void* context::function_pointer(std::string aFunctionName) {
		return vkGetDeviceProcAddr(this->Handle, aFunctionName.c_str());
	}

}
