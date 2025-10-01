#include <geodesy/gpu/command_batch.h>

namespace geodesy::gpu {

    void command_batch::depends_on(std::shared_ptr<semaphore> aSemaphore, VkPipelineStageFlags aWaitStage, std::shared_ptr<command_batch> aWaitBatch) {
        // Add to wait list.
        this->WaitSemaphoreList.push_back(aSemaphore);
        this->WaitStageList.push_back(aWaitStage);
        // Add to other batch's signal list.
        aWaitBatch->SignalSemaphoreList.push_back(aSemaphore);
    }

}