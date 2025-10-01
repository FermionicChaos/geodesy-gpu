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
					Queue[Operation] = { SortedQueueFamilyIndices[i], QueueOffset[SortedQueueFamilyIndices[i]], VK_NULL_HANDLE };
					// Offset into Queue Family.
					QueueOffset[SortedQueueFamilyIndices[i]]++;
					break;
				}
			}
		}

		// Create Structures for device creation from IndexMap.
		std::map<int, int> ReducedIndexMap; // <qfi, queue_count>
		for (const auto& Q : Queue) {
			// For each qfi in IndexMap, count how many times it appears.
			ReducedIndexMap[Q.second.FamilyIndex]++;
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
		for (auto& Q : Queue) {
			vkGetDeviceQueue(this->Handle, Q.second.FamilyIndex, Q.second.Index, &Q.second.Handle);
		}
	}

	context::~context() {
		// Finally destroy device.
		vkDestroyDevice(this->Handle, NULL);
	}

	void* context::function_pointer(std::string aFunctionName) {
		return vkGetDeviceProcAddr(this->Handle, aFunctionName.c_str());
	}

	VkMemoryRequirements context::get_buffer_memory_requirements(VkBuffer aBufferHandle) const {
		VkMemoryRequirements MemoryRequirements;
		vkGetBufferMemoryRequirements(this->Handle, aBufferHandle, &MemoryRequirements);
		return MemoryRequirements;
	}

	VkMemoryRequirements context::get_image_memory_requirements(VkImage aImageHandle) const {
		VkMemoryRequirements MemoryRequirements;
		vkGetImageMemoryRequirements(this->Handle, aImageHandle, &MemoryRequirements);
		return MemoryRequirements;
	}

	// Memory Allocation.
	VkDeviceMemory context::allocate_memory(VkMemoryRequirements aMemoryRequirements, unsigned int aMemoryType, void* aNext) {
		VkResult Result = VK_SUCCESS;
		VkDeviceMemory MemoryHandle = VK_NULL_HANDLE;
		VkMemoryAllocateInfo AllocateInfo{};
		AllocateInfo.sType						= VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		AllocateInfo.pNext 						= aNext;
		AllocateInfo.allocationSize				= aMemoryRequirements.size;
		AllocateInfo.memoryTypeIndex			= this->Device->get_memory_type_index(aMemoryRequirements, aMemoryType);
		Result = vkAllocateMemory(this->Handle, &AllocateInfo, NULL, &MemoryHandle);
		return MemoryHandle;
	}

	void context::free_memory(VkDeviceMemory& aMemoryHandle) {
		vkFreeMemory(this->Handle, aMemoryHandle, NULL);
		aMemoryHandle = VK_NULL_HANDLE;
	}

	context::queue context::get_execution_queue(unsigned int aOperation) {
		// Check if exact match exists.
		if (this->Queue.count(aOperation) > 0) {
			// Exact match found.
			return this->Queue[aOperation];
		}
		else {
			// Check for compatible operations (Op supports aOperation).
			for (const auto& [Op, Q] : this->Queue) {
				if ((Op & aOperation) == aOperation) {
					// Compatible operation found (Op supports all bits in aOperation).
					return Q;
				}
			}
			return { -1, -1, VK_NULL_HANDLE }; // No match found.
		}
	}

	VkResult context::wait() {
		return vkDeviceWaitIdle(this->Handle);
	}

	VkResult context::wait(device::operation aDeviceOperation) {
		queue Q = this->get_execution_queue(aDeviceOperation);
		if (Q.Handle != VK_NULL_HANDLE) {
			return vkQueueWaitIdle(Q.Handle);
		} 
		else {
			return VK_SUCCESS; // No such operation, nothing to wait on.
		}
	}
	
	VkResult context::wait(std::shared_ptr<fence> aFence) {
		std::vector<std::shared_ptr<fence>> FenceList = { aFence };
		return this->wait(FenceList, true);
	}

	VkResult context::wait(std::vector<std::shared_ptr<fence>> aFenceList, VkBool32 aWaitOnAll) {
		std::vector<VkFence> FenceHandleList(aFenceList.size(), VK_NULL_HANDLE);
		for (size_t i = 0; i < aFenceList.size(); i++) {
			FenceHandleList[i] = aFenceList[i]->Handle;
		}
		return vkWaitForFences(this->Handle, aFenceList.size(), FenceHandleList.data(), aWaitOnAll, UINT64_MAX);
	}

	VkResult context::reset(std::shared_ptr<fence> aFence) {
		std::vector<std::shared_ptr<fence>> FenceList = { aFence };
		return this->reset(FenceList);
	}

	VkResult context::reset(std::vector<std::shared_ptr<fence>> aFenceList) {
		std::vector<VkFence> FenceHandleList(aFenceList.size(), VK_NULL_HANDLE);
		for (size_t i = 0; i < aFenceList.size(); i++) {
			FenceHandleList[i] = aFenceList[i]->Handle;
		}
		return vkResetFences(this->Handle, aFenceList.size(), FenceHandleList.data());
	}

	VkResult context::wait_and_reset(std::shared_ptr<fence> aFence) {
		std::vector<std::shared_ptr<fence>> FenceList = { aFence };
		return this->wait_and_reset(FenceList);
	}

	VkResult context::wait_and_reset(std::vector<std::shared_ptr<fence>> aFenceList, VkBool32 aWaitOnAll) {
		VkResult Result = VK_SUCCESS;
		Result = this->wait(aFenceList, aWaitOnAll);
		Result = this->reset(aFenceList);
		return Result;
	}

	VkResult context::execute(device::operation aDeviceOperation, std::shared_ptr<command_buffer> aCommandBuffer, std::shared_ptr<fence> aFence) {
		return this->execute(aDeviceOperation, std::vector<std::shared_ptr<command_buffer>>{ aCommandBuffer }, aFence);
	}

	VkResult context::execute(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_buffer>> aCommandBufferList, std::shared_ptr<fence> aFence) {
		std::shared_ptr<command_batch> Batch(new command_batch(aCommandBufferList));
		return this->execute(aDeviceOperation, std::vector<std::shared_ptr<command_batch>>{ Batch }, aFence);
	}

	VkResult context::execute(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_batch>> aCommandBatchList, std::shared_ptr<fence> aFence) {
		// Check if there is any work to do.
		if (aCommandBatchList.empty()) return VK_SUCCESS;

		// Find a queue for the requested operation.
		context::queue ExecutionQueue = this->get_execution_queue(aDeviceOperation);
		if (ExecutionQueue.Handle == VK_NULL_HANDLE) return VK_ERROR_FEATURE_NOT_PRESENT;

		// Prepare submission structures.
		std::vector<VkSubmitInfo> ExecutionLoad(aCommandBatchList.size());
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
		return vkQueueSubmit(ExecutionQueue.Handle, ExecutionLoad.size(), ExecutionLoad.data(), aFence ? aFence->Handle : VK_NULL_HANDLE);
	}

	VkResult context::execute_and_wait(device::operation aDeviceOperation, std::shared_ptr<command_buffer> aCommandBuffer) {
		return this->execute_and_wait(aDeviceOperation, std::vector<std::shared_ptr<command_buffer>>{ aCommandBuffer });
	}

	VkResult context::execute_and_wait(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_buffer>> aCommandBufferList) {
		std::shared_ptr<command_batch> Batch(new command_batch(aCommandBufferList));
		return this->execute_and_wait(aDeviceOperation, std::vector<std::shared_ptr<command_batch>>{ Batch });
	}

	VkResult context::execute_and_wait(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_batch>> aCommandBatchList) {
		VkResult Result = VK_SUCCESS;
		// Create a fence to wait on.
		auto Fence = this->create<fence>();
		Result = this->execute(aDeviceOperation, aCommandBatchList, Fence);
		if (Result != VK_SUCCESS) {
			return Result;
		}
		Result = this->wait(Fence);
		return Result;		
	}

}
