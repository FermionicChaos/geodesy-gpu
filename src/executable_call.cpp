#include <geodesy/gpu/executable_call.h>

namespace geodesy::gpu {

	executable_call::executable_call() {

	}

	executable_call::executable_call(
		std::shared_ptr<context> 									aContext,
		std::shared_ptr<command_pool> 								aCommandPool,
		std::shared_ptr<pipeline> 									aRasterizationPipeline,
		std::vector<std::shared_ptr<image>> 						aImage,
		std::vector<std::shared_ptr<buffer>> 						aVertexBuffer,
		std::shared_ptr<buffer> 									aIndexBuffer,
		std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding
	) {
		// These objects need to persist longer than the call, so they are passed in.
		this->Context = aContext;
		this->CommandPool = aCommandPool;
		this->Pipeline = aRasterizationPipeline;

	}

}
