#ifndef GEODESY_GPU_FENCE_H
#define GEODESY_GPU_FENCE_H

#include "config.h"

#include "resource.h"

namespace geodesy::gpu {

	class fence : public resource {
	public:

		VkFence Handle;

		fence(std::shared_ptr<context> aContext, bool aSignaled = false);
		~fence();

	};
	
}

#endif // !GEODESY_GPU_FENCE_H
