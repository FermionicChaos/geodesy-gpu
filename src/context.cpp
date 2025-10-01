#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	// Internal Execution Submission Structure
	struct __backend_gpu_submission {
		std::vector<VkCommandBuffer> 				CommandBufferList;
		std::vector<VkSemaphore> 					WaitSemaphoreList;
		std::vector<VkSemaphore> 					SignalSemaphoreList;
	};

	context::context() {
		this->Instance = nullptr;
		this->Device = nullptr;
		this->Handle = VK_NULL_HANDLE;
		this->vkGetDeviceProcAddr = NULL;
	}

	context::context(
		std::shared_ptr<instance> 		aInstance,
		std::shared_ptr<device> 		aDevice,
		std::vector<unsigned int> 		aOperations,
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
		for (auto& Operation : aOperations) {
			// Get list of sorted indices starting with qfis most similar to desired operations.
			std::vector<int> SortedQueueFamilyIndices = aDevice->sort_queue_family_indices(Operation);
			if (SortedQueueFamilyIndices.size() == 0) {
				throw std::runtime_error("Device does not support requested operation.");
			}

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

		// Post creation, load queue handles.
		for (const auto& [Op, ij] : IndexMap) {
			this->Queue[Op] = VK_NULL_HANDLE;
			vkGetDeviceQueue(this->Handle, ij.first, ij.second, &this->Queue[Op]);
		}
	}

	context::~context() {
		// Finally destroy device.
		vkDestroyDevice(this->Handle, NULL);
	}

	void* context::function_pointer(std::string aFunctionName) {
		return vkGetDeviceProcAddr(this->Handle, aFunctionName.c_str());
	}

	VkResult context::execute(device::operation aDeviceOperation, std::shared_ptr<command_buffer> aCommandBuffer, std::shared_ptr<fence> aFence) {
		return this->execute(aDeviceOperation, std::vector<std::shared_ptr<command_buffer>>{ aCommandBuffer }, aFence);
	}

	VkResult context::execute(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_buffer>> aCommandBufferList, std::shared_ptr<fence> aFence) {
		std::shared_ptr<command_batch> Batch(new command_batch(aCommandBufferList));
		return this->execute(aDeviceOperation, std::vector<std::shared_ptr<command_batch>>{ Batch }, aFence);
	}

	VkResult context::execute(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_batch>> aCommandBatchList, std::shared_ptr<fence> aFence) {
		// Validate input
		if (aCommandBatchList.empty()) {
			return VK_SUCCESS; // Nothing to submit
		}

		VkQueue ExecutionQueue = VK_NULL_HANDLE;
		std::vector<VkSubmitInfo> ExecutionLoad(aCommandBatchList.size());

		// First find a queue for the requested operation.
		if (this->Queue.count(aDeviceOperation) > 0) {
			ExecutionQueue = this->Queue[aDeviceOperation];
		}
		else {
			return VK_ERROR_FEATURE_NOT_PRESENT;
		}

		// Populate submission structures.
		std::vector<__backend_gpu_submission> Submissions(aCommandBatchList.size());
		for (size_t i = 0; i < aCommandBatchList.size(); i++) {
			Submissions[i].CommandBufferList = std::vector<VkCommandBuffer>(aCommandBatchList[i]->CommandBufferList.size());
			for (size_t j = 0; j < aCommandBatchList[i]->CommandBufferList.size(); j++) {
				Submissions[i].CommandBufferList[j] = aCommandBatchList[i]->CommandBufferList[j]->Handle;
			}
			Submissions[i].WaitSemaphoreList = std::vector<VkSemaphore>(aCommandBatchList[i]->WaitSemaphoreList.size());
			for (size_t j = 0; j < aCommandBatchList[i]->WaitSemaphoreList.size(); j++) {
				Submissions[i].WaitSemaphoreList[j] = aCommandBatchList[i]->WaitSemaphoreList[j]->Handle;
			}
			Submissions[i].SignalSemaphoreList = std::vector<VkSemaphore>(aCommandBatchList[i]->SignalSemaphoreList.size());
			for (size_t j = 0; j < aCommandBatchList[i]->SignalSemaphoreList.size(); j++) {
				Submissions[i].SignalSemaphoreList[j] = aCommandBatchList[i]->SignalSemaphoreList[j]->Handle;
			}
		}

		// Finalize into VkSubmitInfo structures.
		for (size_t i = 0; i < aCommandBatchList.size(); i++) {
			ExecutionLoad[i].sType						= VK_STRUCTURE_TYPE_SUBMIT_INFO;
			ExecutionLoad[i].pNext						= NULL;
			ExecutionLoad[i].waitSemaphoreCount			= Submissions[i].WaitSemaphoreList.size();
			ExecutionLoad[i].pWaitSemaphores			= Submissions[i].WaitSemaphoreList.data();
			ExecutionLoad[i].pWaitDstStageMask			= aCommandBatchList[i]->WaitStageList.data();
			ExecutionLoad[i].commandBufferCount			= Submissions[i].CommandBufferList.size();
			ExecutionLoad[i].pCommandBuffers			= Submissions[i].CommandBufferList.data();
			ExecutionLoad[i].signalSemaphoreCount		= Submissions[i].SignalSemaphoreList.size();
			ExecutionLoad[i].pSignalSemaphores			= Submissions[i].SignalSemaphoreList.data();
		}

		// Execute workload on the device.
		return vkQueueSubmit(ExecutionQueue, ExecutionLoad.size(), ExecutionLoad.data(), aFence ? aFence->Handle : VK_NULL_HANDLE);
	}


}
