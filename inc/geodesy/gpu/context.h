#ifndef GEODESY_GPU_CONTEXT_H
#define GEODESY_GPU_CONTEXT_H

#include "config.h"

#include "device.h"
#include "fence.h"
#include "semaphore.h"

namespace geodesy::gpu {

	class device;

	class context : public std::enable_shared_from_this<context> {
	public:

		// Insure that these objects outlive context.
		std::shared_ptr<instance> 							Instance;
		std::shared_ptr<device> 							Device;

		std::map<unsigned int, std::pair<int, int>> 		IndexMap;
		std::map<unsigned int, VkQueue> 					Queue;
		VkDevice 											Handle;

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

		void* function_pointer(std::string aFunctionName);

		// VkResult execute(device::operation aDeviceOperation, const std::shared_ptr<command_buffer>& aCommandBuffer, VkFence aFence = VK_NULL_HANDLE);
		// VkResult execute(device::operation aDeviceOperation, const std::shared_ptr<command_batch>& aSubmission, VkFence aFence = VK_NULL_HANDLE);
		// VkResult execute(device::operation aDeviceOperation, const std::vector<std::shared_ptr<command_batch>>& aSubmissionList, VkFence aFence = VK_NULL_HANDLE);

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