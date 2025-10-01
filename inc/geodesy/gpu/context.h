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

		VkResult execute(device::operation aDeviceOperation, std::shared_ptr<command_buffer> aCommandBuffer, std::shared_ptr<fence> aFence = nullptr);
		VkResult execute(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_buffer>> aCommandBufferList, std::shared_ptr<fence> aFence = nullptr);
		VkResult execute(device::operation aDeviceOperation, std::vector<std::shared_ptr<command_batch>> aCommandBatchList, std::shared_ptr<fence> aFence = nullptr);

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