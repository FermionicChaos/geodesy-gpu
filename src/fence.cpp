#include <geodesy/gpu/fence.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	fence::fence(std::shared_ptr<context> aContext, bool aSignaled) {
		VkResult Result = VK_SUCCESS;
		this->Context = aContext;

		VkFenceCreateInfo FCI = {};
		FCI.sType			= VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		FCI.pNext			= NULL;
		FCI.flags			= aSignaled ? VK_FENCE_CREATE_SIGNALED_BIT : 0;

		Result = vkCreateFence(this->Context->Handle, &FCI, NULL, &this->Handle);
		if (Result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create fence.");
		}
	}

	fence::~fence() {
		// It is automatically assumed that Context and Handle are valid.
		vkDestroyFence(this->Context->Handle, this->Handle, NULL);
	}
	
}
