#include <geodesy/gpu/command_pool.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {
	
	command_pool::~command_pool() {
		// It is automatically assumed that Context and Handle are valid.
		// Because every command buffer has a shared copy of the command pool,
		// this won't be called until all command buffers are destroyed.
		vkDestroyCommandPool(this->Context->Handle, this->Handle, NULL);
	}

	std::shared_ptr<command_buffer> command_pool::allocate_command_buffer(VkCommandBufferLevel aLevel) {
		return nullptr;
	}
	
	std::vector<std::shared_ptr<command_buffer>> command_pool::allocate_command_buffer(size_t aCount, VkCommandBufferLevel aLevel) {
		VkResult Result = VK_SUCCESS;
		std::vector<std::shared_ptr<command_buffer>> CommandBuffers;
		std::vector<VkCommandBuffer> CB(aCount, VK_NULL_HANDLE);
		VkCommandBufferAllocateInfo CBAI = {};
		CBAI.sType						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CBAI.pNext						= NULL;
		CBAI.commandPool				= this->Handle;
		CBAI.level						= aLevel;
		CBAI.commandBufferCount			= aCount;
		Result = vkAllocateCommandBuffers(this->Context->Handle, &CBAI, NULL, CB.data());
		if (Result == VK_SUCCESS) {
			for (auto& buffer : CB) {
				CommandBuffers.push_back(std::make_shared<command_buffer>(this->Context, buffer));
			}
		}
		else {
			return CommandBuffers;
		}
	}
	
	
}