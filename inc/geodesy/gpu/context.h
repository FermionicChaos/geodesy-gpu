#ifndef GEODESY_GPU_CONTEXT_H
#define GEODESY_GPU_CONTEXT_H

#include "config.h"

namespace geodesy::gpu {

	class device;

	class context : public std::enable_shared_from_this<context> {
	public:

		VkDevice Handle;

		// context();
		// context();
		// ~context();

	private:

	};

}

#endif // !GEODESY_GPU_CONTEXT_H