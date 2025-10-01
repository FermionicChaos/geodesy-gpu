#include <geodesy/gpu/semaphore.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {
	
	semaphore::~semaphore() {
		// It is automatically assumed that Context and Handle are valid.
		vkDestroySemaphore(this->Context->Handle, this->Handle, NULL);
	}
	
}