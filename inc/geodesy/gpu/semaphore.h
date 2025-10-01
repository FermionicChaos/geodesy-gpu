#ifndef GEODESY_GPU_SEMAPHORE_H
#define GEODESY_GPU_SEMAPHORE_H

#include "config.h"

#include "resource.h"

namespace geodesy::gpu {

    class semaphore : public resource {
    public:

        VkSemaphore Handle;

        ~semaphore();
        
    };
    
}

#endif // !GEODESY_GPU_SEMAPHORE_H
