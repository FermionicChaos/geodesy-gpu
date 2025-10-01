#include <geodesy/gpu/fence.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	fence::~fence() {
		// It is automatically assumed that Context and Handle are valid.
		vkDestroyFence(this->Context->Handle, this->Handle, NULL);
	}
	
}
