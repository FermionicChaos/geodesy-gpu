#ifndef GEODESY_GPU_CONTEXT_H
#define GEODESY_GPU_CONTEXT_H

#include "config.h"

#include "device.h"

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

	private:

		PFN_vkGetDeviceProcAddr vkGetDeviceProcAddr;

	};

}

#endif // !GEODESY_GPU_CONTEXT_H