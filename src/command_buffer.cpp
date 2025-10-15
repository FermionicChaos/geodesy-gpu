#include <geodesy/gpu/command_buffer.h>

#include <geodesy/gpu/command_pool.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	command_buffer::command_buffer() {
		this->Type = resource::type::COMMAND_BUFFER;
		this->CommandPool = nullptr;
		this->Handle = VK_NULL_HANDLE;
	}

	command_buffer::command_buffer(std::shared_ptr<context> aContext, std::shared_ptr<command_pool> aCommandPool, VkCommandBuffer aHandle) : command_buffer() {
		this->Context = aContext;
		this->CommandPool = aCommandPool;
		this->Handle = aHandle;
	}

	command_buffer::command_buffer(std::shared_ptr<context> aContext, std::shared_ptr<command_pool> aCommandPool, VkCommandBufferLevel aLevel) : command_buffer() {
		VkResult Result = VK_SUCCESS;
		VkCommandBufferAllocateInfo CBAI = {};
		PFN_vkAllocateCommandBuffers vkAllocateCommandBuffers = (PFN_vkAllocateCommandBuffers)this->Context->function_pointer("vkAllocateCommandBuffers");
		CBAI.sType						= VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		CBAI.pNext						= NULL;
		CBAI.commandPool				= aCommandPool->Handle;
		CBAI.level						= aLevel;
		CBAI.commandBufferCount			= 1;
		Result = vkAllocateCommandBuffers(this->Context->Handle, &CBAI, &this->Handle);
	}

	command_buffer::~command_buffer() {
		PFN_vkFreeCommandBuffers vkFreeCommandBuffers = (PFN_vkFreeCommandBuffers)this->Context->function_pointer("vkFreeCommandBuffers");
		// It is automatically assumed that Context and Handle are valid.
		vkFreeCommandBuffers(this->Context->Handle, this->CommandPool->Handle, 1, &this->Handle);
	}

	VkResult command_buffer::begin() {
		VkCommandBufferBeginInfo BeginInfo{};
		PFN_vkBeginCommandBuffer vkBeginCommandBuffer = (PFN_vkBeginCommandBuffer)this->Context->function_pointer("vkBeginCommandBuffer");
		BeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		BeginInfo.pNext = NULL;
		BeginInfo.flags = 0;
		BeginInfo.pInheritanceInfo = NULL;
		return vkBeginCommandBuffer(this->Handle, &BeginInfo);
	}
	
	VkResult command_buffer::end() {
		PFN_vkEndCommandBuffer vkEndCommandBuffer = (PFN_vkEndCommandBuffer)this->Context->function_pointer("vkEndCommandBuffer");
		return vkEndCommandBuffer(this->Handle);
	}

	void command_buffer::bind_vertex_buffers(VkCommandBuffer aCommandBuffer, std::vector<VkBuffer> aBufferList, const VkDeviceSize* aOffset) {
		PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)this->Context->function_pointer("vkCmdBindVertexBuffers");
		vkCmdBindVertexBuffers(aCommandBuffer, 0, aBufferList.size(), aBufferList.data(), aOffset);
	}

	void command_buffer::bind_index_buffer(VkCommandBuffer aCommandBuffer, VkBuffer aBufferHandle, VkIndexType aIndexType) {
		PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)this->Context->function_pointer("vkCmdBindIndexBuffer");
		vkCmdBindIndexBuffer(aCommandBuffer, aBufferHandle, 0, aIndexType);
	}

	void command_buffer::bind_descriptor_sets(VkCommandBuffer aCommandBuffer, VkPipelineBindPoint aPipelineBindPoint, VkPipelineLayout aPipelineLayout, std::vector<VkDescriptorSet> aDescriptorSetList, std::vector<uint32_t> aDynamicOffsetList) {
		PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)this->Context->function_pointer("vkCmdBindDescriptorSets");
		vkCmdBindDescriptorSets(aCommandBuffer, aPipelineBindPoint, aPipelineLayout, 0, aDescriptorSetList.size(), aDescriptorSetList.data(), aDynamicOffsetList.size(), aDynamicOffsetList.data());
	}

	void command_buffer::bind_pipeline(VkCommandBuffer aCommandBuffer, VkPipelineBindPoint aPipelineBindPoint, VkPipeline aPipelineHandle) {
		PFN_vkCmdBindPipeline vkCmdBindPipeline = (PFN_vkCmdBindPipeline)this->Context->function_pointer("vkCmdBindPipeline");
		vkCmdBindPipeline(aCommandBuffer, aPipelineBindPoint, aPipelineHandle);
	}

	void command_buffer::draw_indexed(VkCommandBuffer aCommandBuffer, uint32_t aIndexCount, uint32_t aInstanceCount, uint32_t aFirstIndex, uint32_t aVertexOffset, uint32_t aFirstInstance) {
		PFN_vkCmdDrawIndexed vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)this->Context->function_pointer("vkCmdDrawIndexed");
		vkCmdDrawIndexed(aCommandBuffer, aIndexCount, aInstanceCount, aFirstIndex, aVertexOffset, aFirstInstance);
	}

}
