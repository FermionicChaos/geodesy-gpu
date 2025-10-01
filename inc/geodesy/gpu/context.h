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

		// template<typename T>
		// std::vector<std::shared_ptr<T>> create_resource(size_t aCount) {
		// 	std::vector<std::shared_ptr<T>> Resources;
		// 	for (size_t i = 0; i < aCount; i++) {
		// 		std::shared_ptr<T> NewResource;
		// 		NewResource = geodesy::make<T>(this->shared_from_this());
		// 		if (NewResource) {
		// 			Resources.push_back(NewResource);
		// 		}
		// 	}
		// 	return Resources;
		// }

	private:

		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

	};

}

#endif // !GEODESY_GPU_CONTEXT_H