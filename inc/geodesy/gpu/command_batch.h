#ifndef GEODESY_GPU_COMMAND_BATCH_H
#define GEODESY_GPU_COMMAND_BATCH_H

#include "config.h"

#include "device.h"
#include "resource.h"
#include "fence.h"
#include "semaphore.h"
#include "semaphore_pool.h"
#include "command_buffer.h"
#include "command_pool.h"

namespace geodesy::gpu {

	// This class is only a container to build VkSubmitInfo structures for queue submission.
	class command_batch {
	public:
		
		std::vector<std::shared_ptr<command_buffer>> 	CommandBufferList;
		std::vector<std::shared_ptr<semaphore>> 		WaitSemaphoreList;
		std::vector<VkPipelineStageFlags> 				WaitStageList;
		std::vector<std::shared_ptr<semaphore>> 		SignalSemaphoreList;

		command_batch();
		command_batch(std::vector<std::shared_ptr<command_buffer>> aCommandBufferList);
		~command_batch();

		// Create subscript operators for command buffer access.
		std::shared_ptr<command_buffer>& operator[](size_t aIndex);
		const std::shared_ptr<command_buffer>& operator[](size_t aIndex) const;

		void depends_on(std::shared_ptr<semaphore> aSemaphore, VkPipelineStageFlags aWaitStage, std::shared_ptr<command_batch> aWaitBatch);

	};

}

#endif // !GEODESY_GPU_COMMAND_BATCH_H
