#ifndef GEODESY_GPU_RESOURCE_H
#define GEODESY_GPU_RESOURCE_H

#include "config.h"

namespace geodesy::gpu {

	class resource {
	public:

		// These are the base resources that the GPU and create/destroy.
		enum type : unsigned int {
			UNKNOWN,
			FENCE,
			SEMAPHORE,
			COMMAND_BUFFER,
			COMMAND_POOL,
			BUFFER,
			IMAGE,
			ACCELERATION_STRUCTURE,
			// SHADER,
			DESCRIPTOR,
			FRAMEBUFFER,
			PIPELINE
		};

		std::shared_ptr<context>        Context;
		type                            Type;

		// Virtual destructor to make the class polymorphic
		virtual ~resource() = default;

	protected:
		
		resource();
		
	};

}

#endif // !GEODESY_GPU_RESOURCE_H
