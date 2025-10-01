#ifndef GEODESY_GPU_RESOURCE_H
#define GEODESY_GPU_RESOURCE_H

#include "config.h"

namespace geodesy::gpu {

    class resource {
    public:

        std::shared_ptr<context>        Context;
        
        resource();
        
    };

}

#endif // !GEODESY_GPU_RESOURCE_H
