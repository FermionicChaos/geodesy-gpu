#ifndef GEODESY_GPU_COMMAND_POOL_H
#define GEODESY_GPU_COMMAND_POOL_H

#include "config.h"

#include "command_buffer.h"

namespace geodesy::gpu {

	class command_pool {
	public:

		std::shared_ptr<context>        Context;

		VkCommandPool                   Handle;

		~command_pool();

		std::shared_ptr<command_buffer> allocate_command_buffer(VkCommandBufferLevel aLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		std::vector<std::shared_ptr<command_buffer>> allocate_command_buffer(size_t aCount = 1, VkCommandBufferLevel aLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);

	};
	
}

#endif // !GEODESY_GPU_COMMAND_POOL_H