#ifndef GEODESY_GPU_SEMAPHORE_H
#define GEODESY_GPU_SEMAPHORE_H

#include "config.h"

namespace geodesy::gpu {

    class semaphore {
    public:

        std::shared_ptr<context> Context;
        
        VkSemaphore Handle;

        ~semaphore();
        
    };
    
}

#endif // !GEODESY_GPU_SEMAPHORE_H
