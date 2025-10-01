#include <geodesy/gpu/semaphore.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	semaphore::semaphore() {
		this->Context = nullptr;
		this->Handle = VK_NULL_HANDLE;
	}

	semaphore::semaphore(std::shared_ptr<context> aContext) : semaphore() {
		VkSemaphoreCreateInfo SCI = {};
		SCI.sType						= VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		SCI.pNext						= NULL;
		SCI.flags						= 0;

		VkResult Result = vkCreateSemaphore(this->Context->Handle, &SCI, NULL, &this->Handle);
		if (Result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create semaphore.");
		}
	}
	
	semaphore::~semaphore() {
		// It is automatically assumed that Context and Handle are valid.
		vkDestroySemaphore(this->Context->Handle, this->Handle, NULL);
	}
	
}