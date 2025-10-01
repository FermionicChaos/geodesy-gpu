#ifndef GEODESY_GPU_CONTEXT_H
#define GEODESY_GPU_CONTEXT_H

#include "config.h"

#include "device.h"

namespace geodesy::gpu {

	class device;

	class context : public std::enable_shared_from_this<context> {
	public:

		std::shared_ptr<instance> 		Instance;
		std::shared_ptr<device> 		Device;

		VkDevice 						Handle;

		context();
		context(
			std::shared_ptr<instance> 	aInstance,
			std::shared_ptr<device> 	aDevice,
			std::vector<int> 			aExecutionOperations,
			std::set<std::string> 		aLayers,
			std::set<std::string> 		aExtensions,
			void* 						aNext = NULL
		);
		~context();

	private:

	};

}

#endif // !GEODESY_GPU_CONTEXT_H