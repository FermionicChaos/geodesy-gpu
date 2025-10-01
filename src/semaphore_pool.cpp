#include <geodesy/gpu/semaphore_pool.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	VkSemaphore semaphore_pool::acquire() {
		VkSemaphore Semaphore = this->Available.front();
		this->Available.pop();
		this->InUse.insert(Semaphore);
		return Semaphore;
	}

	void semaphore_pool::release(VkSemaphore aSemaphore) {
		// If does not belong to this pool, ignore. Does not own it.
		if (this->InUse.count(aSemaphore) == 0) return;
		// Add semaphore back to pool.
		this->Available.push(aSemaphore);
		// Remove from in use list.
		this->InUse.erase(aSemaphore);
	}

	void semaphore_pool::reset() {
		for (VkSemaphore Semaphore : this->InUse) {
			this->Available.push(Semaphore);
		}
		this->InUse.clear();
	}

}
