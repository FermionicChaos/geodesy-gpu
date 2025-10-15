#include <geodesy/gpu/command_pool.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	command_pool::command_pool() {
		this->Handle = VK_NULL_HANDLE;
		this->Type = resource::type::COMMAND_POOL;
	}

	command_pool::command_pool(std::shared_ptr<context> aContext, unsigned int aOperation, VkCommandPoolCreateFlags aFlags) : command_pool() {
		PFN_vkCreateCommandPool vkCreateCommandPool = (PFN_vkCreateCommandPool)aContext->function_pointer("vkCreateCommandPool");
		
		this->Context = aContext;

		context::queue Q = aContext->get_execution_queue(aOperation);
		VkCommandPoolCreateInfo CPCI = {};
		CPCI.sType						= VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		CPCI.pNext						= NULL;
		CPCI.flags						= aFlags;
		CPCI.queueFamilyIndex			= Q.FamilyIndex;

		VkResult Result = vkCreateCommandPool(aContext->Handle, &CPCI, NULL, &this->Handle);
		if (Result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create command pool.");
		}
	}

	command_pool::~command_pool() {
		PFN_vkDestroyCommandPool vkDestroyCommandPool = (PFN_vkDestroyCommandPool)this->Context->function_pointer("vkDestroyCommandPool");
		// It is automatically assumed that Context and Handle are valid.
		// Because every command buffer has a shared copy of the command pool,
		// this won't be called until all command buffers are destroyed.
		vkDestroyCommandPool(this->Context->Handle, this->Handle, NULL);
	}	
	
}