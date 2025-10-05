#include <geodesy/gpu/framechain.h>

namespace geodesy::gpu {

	framechain::framechain() {
		this->Context = nullptr;
		this->FrameRate = 60.0;
		this->Resolution = { 0, 0, 1 };
		this->ReadIndex = 0;
		this->DrawIndex = 0;
	}
	
	// This function is special because it presents, and acquires next frame.
	VkResult framechain::next_frame(VkSemaphore aPresentFrameSemaphore, VkSemaphore aNextFrameSemaphore, VkFence aNextFrameFence) {
		this->ReadIndex = this->DrawIndex;
		this->DrawIndex = (this->DrawIndex + 1) % this->Image.size();
		return VK_SUCCESS;
	}

	VkResult framechain::present_frame_now() {
		return VK_SUCCESS;
	}	

}
