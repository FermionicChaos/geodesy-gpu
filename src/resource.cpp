#include <geodesy/gpu/resource.h>

namespace geodesy::gpu {

    resource::resource() {
        this->Context   = nullptr;
        this->Type      = resource::type::UNKNOWN;
    }
    
}