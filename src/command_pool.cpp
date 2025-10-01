#include <geodesy/gpu/command_pool.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	command_pool::command_pool() {
		this->Handle = VK_NULL_HANDLE;
	}

	command_pool::command_pool(std::shared_ptr<context> aContext, uint32_t aQueueFamilyIndex, VkCommandPoolCreateFlags aFlags) : command_pool() {
		this->Context = aContext;

		VkCommandPoolCreateInfo CPCI = {};
		CPCI.sType						= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		CPCI.pNext						= NULL;
		CPCI.flags						= aFlags;
		CPCI.queueFamilyIndex			= aQueueFamilyIndex;

		VkResult Result = vkCreateCommandPool(aContext->Handle, &CPCI, NULL, &this->Handle);
		if (Result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create command pool.");
		}
	}

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
		Result = vkAllocateCommandBuffers(this->Context->Handle, &CBAI, CB.data());
		if (Result == VK_SUCCESS) {
			for (auto& buffer : CB) {
				CommandBuffers.push_back(geodesy::make<command_buffer>(this->Context, this->shared_from_this(), buffer));
			}
		}
		return CommandBuffers;
	}
	
	
}