#ifndef GEODESY_GPU_FENCE_H
#define GEODESY_GPU_FENCE_H

#include "config.h"

namespace geodesy::gpu {

	class fence {
	public:

		std::shared_ptr<context> Context;

		VkFence Handle;

		~fence();

	};
	
}

#endif // !GEODESY_GPU_FENCE_H
