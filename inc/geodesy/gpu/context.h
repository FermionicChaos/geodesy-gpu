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

		// Generic resource creation with variadic template arguments
		template<typename T, typename... Args>
		std::shared_ptr<T> create(Args&&... args) {
			return geodesy::make<T>(this->shared_from_this(), std::forward<Args>(args)...);
		}

		// // Create multiple resources of the same type
		// template<typename T, typename... Args>
		// std::vector<std::shared_ptr<T>> create_multiple(size_t count, Args&&... args) {
		// 	std::vector<std::shared_ptr<T>> resources;
		// 	resources.reserve(count);
			
		// 	for (size_t i = 0; i < count; i++) {
		// 		auto resource = create<T>(std::forward<Args>(args)...);
		// 		if (resource) {
		// 			resources.push_back(resource);
		// 		}
		// 	}
			
		// 	return resources;
		// }

	private:

		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

	};

}

#endif // !GEODESY_GPU_CONTEXT_H