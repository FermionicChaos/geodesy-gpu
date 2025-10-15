#ifndef GEODESY_GPU_COMMAND_POOL_H
#define GEODESY_GPU_COMMAND_POOL_H

#include "config.h"

#include "resource.h"
#include "command_buffer.h"

namespace geodesy::gpu {

	class command_pool : public std::enable_shared_from_this<command_pool>, public resource {
	public:

		VkCommandPool                   Handle;

		command_pool();
		command_pool(std::shared_ptr<context> aContext, unsigned int aOperation, VkCommandPoolCreateFlags aFlags = 0);
		~command_pool();

		template<typename T, typename... Args>
		std::shared_ptr<T> create(Args&&... args) {
			return geodesy::make<T>(this->Context, this->shared_from_this(), std::forward<Args>(args)...);
		}

	};
	
}

#endif // !GEODESY_GPU_COMMAND_POOL_H