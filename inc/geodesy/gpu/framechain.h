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
		std::array<unsigned int, 3>													Resolution;
		uint32_t																	ReadIndex;
		uint32_t																	DrawIndex;
		double																		FrameRate;
		std::vector<std::map<std::string, std::shared_ptr<image>>> 					Image;

		framechain(std::shared_ptr<context> aContext, double aFrameRate, uint32_t aFrameCount);

		std::map<std::string, std::shared_ptr<image>> read_frame();
		std::map<std::string, std::shared_ptr<image>> draw_frame();
		bool ready_to_render();
		VkResult next_frame_now();
		VkResult present_frame_now();
		
		// This function is special because it presents, and acquires next frame.
		virtual VkResult next_frame(VkSemaphore aPresentFrameSemaphore = VK_NULL_HANDLE, VkSemaphore aNextFrameSemaphore = VK_NULL_HANDLE, VkFence aNextFrameFence = VK_NULL_HANDLE);
		virtual std::vector<command_batch> predraw();
		virtual std::vector<command_batch> postdraw();

	};

}

#endif // !GEODESY_GPU_FRAMECHAIN_H
