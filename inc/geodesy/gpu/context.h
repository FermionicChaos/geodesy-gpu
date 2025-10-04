#ifndef GEODESY_GPU_CONTEXT_H
#define GEODESY_GPU_CONTEXT_H

#include "config.h"

#include "device.h"
#include "resource.h"
#include "fence.h"
#include "semaphore.h"
#include "semaphore_pool.h"
#include "command_buffer.h"
#include "command_pool.h"
#include "command_batch.h"
#include "buffer.h"
#include "image.h"
#include "shader.h"
#include "descriptor.h"
#include "framebuffer.h"
#include "acceleration_structure.h"
#include "pipeline.h"
#include "framechain.h"
#include "swapchain.h"

namespace geodesy::gpu {

	class device;

	class context : public std::enable_shared_from_this<context> {
	public:

		struct queue {
			int 		FamilyIndex;
			int 		Index;
			VkQueue 	Handle;
		};

		// Insure that these objects outlive context.
		std::shared_ptr<instance> 								Instance;
		std::shared_ptr<device> 								Device;
		
		std::map<unsigned int, queue> 							Queue;
		VkDevice 												Handle;

		context();
		context(
			std::shared_ptr<instance> 		aInstance,
			std::shared_ptr<device> 		aDevice,
			std::vector<unsigned int> 		aOperations,
			std::set<std::string> 			aLayers = {},
			std::set<std::string> 			aExtensions = {},
			void* 							aNext = NULL
		);
		~context();

		void* function_pointer(std::string aFunctionName) const;

		// Memory Allocation Tools
		VkMemoryRequirements get_buffer_memory_requirements(VkBuffer aBufferHandle) const;
		VkMemoryRequirements get_image_memory_requirements(VkImage aImageHandle) const;
		VkDeviceMemory allocate_memory(VkMemoryRequirements aMemoryRequirements, unsigned int aMemoryType, void* aNext = NULL);
		void free_memory(VkDeviceMemory& aMemoryHandle);

		queue get_execution_queue(unsigned int aOperation);

		// Wait on device to become idle.
		VkResult wait();
		// Wait on specific queue to become idle.
		VkResult wait(device::operation aDeviceOperation);
		// Wait on specific fence to become signaled.
		VkResult wait(std::shared_ptr<fence> aFence);
		VkResult wait(std::vector<std::shared_ptr<fence>> aFenceList, VkBool32 aWaitOnAll = VK_TRUE);

		VkResult reset(std::shared_ptr<fence> aFence);
		VkResult reset(std::vector<std::shared_ptr<fence>> aFenceList);

		VkResult wait_and_reset(std::shared_ptr<fence> aFence);
		VkResult wait_and_reset(std::vector<std::shared_ptr<fence>> aFenceList, VkBool32 aWaitOnAll = VK_TRUE);

		VkResult execute(device::operation aDeviceOperation, std::shared_ptr<command_buffer> aCommandBuffer, std::shared_ptr<fence> aFence = nullptr);
		VkResult execute(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_buffer>> aCommandBufferList, std::shared_ptr<fence> aFence = nullptr);
		VkResult execute(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_batch>> aCommandBatchList, std::shared_ptr<fence> aFence = nullptr);

		VkResult execute_and_wait(device::operation aDeviceOperation, std::shared_ptr<command_buffer> aCommandBuffer);
		VkResult execute_and_wait(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_buffer>> aCommandBufferList);
		VkResult execute_and_wait(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_batch>> aCommandBatchList);

		// Generic resource creation with variadic template arguments
		template<typename T, typename... Args>
		std::shared_ptr<T> create(Args&&... args) {
			return geodesy::make<T>(this->shared_from_this(), std::forward<Args>(args)...);
		}

	private:

		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

	};

}

#endif // !GEODESY_GPU_CONTEXT_H