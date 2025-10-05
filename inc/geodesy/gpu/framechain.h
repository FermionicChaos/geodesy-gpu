#ifndef GEODESY_GPU_FRAMECHAIN_H
#define GEODESY_GPU_FRAMECHAIN_H

#include "config.h"

#include "device.h"
#include "resource.h"
#include "command_buffer.h"
#include "command_pool.h"
#include "command_batch.h"
#include "image.h"

namespace geodesy::gpu {

	class framechain {
	public:

		std::shared_ptr<context> 													Context;
		double																		FrameRate;
		std::vector<std::map<std::string, std::shared_ptr<image>>> 					Image;
		std::array<unsigned int, 3>													Resolution;

		uint32_t																	ReadIndex;
		uint32_t																	DrawIndex;

		framechain();
		
		// Returns two semaphores:
		// First: The semaphore used to wait for the next frame to be ready for reading.
		// Second: The semaphore that will be used to signal presentation when rendering is complete.
		virtual VkResult next_frame();
		virtual std::pair<std::shared_ptr<semaphore>, std::shared_ptr<semaphore>> get_acquire_present_semaphore_pair();

	};

}

#endif // !GEODESY_GPU_FRAMECHAIN_H
