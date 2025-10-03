#include <geodesy/gpu/command_buffer.h>

#include <geodesy/gpu/command_pool.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	command_buffer::command_buffer() {
		this->CommandPool = nullptr;
		this->Handle = VK_NULL_HANDLE;
	}

	command_buffer::command_buffer(std::shared_ptr<context> aContext, std::shared_ptr<command_pool> aCommandPool, VkCommandBuffer aHandle) : command_buffer() {
		this->Context = aContext;
		this->CommandPool = aCommandPool;
		this->Handle = aHandle;
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

	void command_buffer::begin_rendering(VkRect2D aRenderArea, std::vector<VkImageView> aColorAttachments, VkImageView aDepthAttachment, VkImageView aStencilAttachment) {
		std::vector<VkRenderingAttachmentInfo> ColorAttachmentInfo(aColorAttachments.size());
		VkRenderingAttachmentInfo DepthAttachmentInfo{};
		VkRenderingAttachmentInfo StencilAttachmentInfo{};
		VkRenderingInfo RenderingInfo{};
		PFN_vkCmdBeginRendering vkCmdBeginRendering = (PFN_vkCmdBeginRendering)this->Context->function_pointer("vkCmdBeginRendering");

		for (size_t i = 0; i < aColorAttachments.size(); i++) {
			ColorAttachmentInfo[i].sType				= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
			ColorAttachmentInfo[i].pNext				= NULL;
			ColorAttachmentInfo[i].imageView			= aColorAttachments[i];
			ColorAttachmentInfo[i].imageLayout			= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			ColorAttachmentInfo[i].resolveMode			= VK_RESOLVE_MODE_NONE;
			ColorAttachmentInfo[i].resolveImageView		= VK_NULL_HANDLE;
			ColorAttachmentInfo[i].resolveImageLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
			ColorAttachmentInfo[i].loadOp				= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			ColorAttachmentInfo[i].storeOp				= VK_ATTACHMENT_STORE_OP_STORE;
			//ColorAttachmentInfo[i].clearValue			= { 0.0f, 0.0f, 0.0f, 0.0f };
		}

		DepthAttachmentInfo.sType					= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		DepthAttachmentInfo.pNext					= NULL;
		DepthAttachmentInfo.imageView				= aDepthAttachment;
		DepthAttachmentInfo.imageLayout				= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		DepthAttachmentInfo.resolveMode				= VK_RESOLVE_MODE_NONE;
		DepthAttachmentInfo.resolveImageView		= VK_NULL_HANDLE;
		DepthAttachmentInfo.resolveImageLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
		DepthAttachmentInfo.loadOp					= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		DepthAttachmentInfo.storeOp					= VK_ATTACHMENT_STORE_OP_STORE;
		//DepthAttachmentInfo.clearValue				= { 0.0f, 0.0f, 0.0f, 0.0f };

		StencilAttachmentInfo.sType					= VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
		StencilAttachmentInfo.pNext					= NULL;
		StencilAttachmentInfo.imageView				= aDepthAttachment;
		StencilAttachmentInfo.imageLayout			= VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		StencilAttachmentInfo.resolveMode			= VK_RESOLVE_MODE_NONE;
		StencilAttachmentInfo.resolveImageView		= VK_NULL_HANDLE;
		StencilAttachmentInfo.resolveImageLayout	= VK_IMAGE_LAYOUT_UNDEFINED;
		StencilAttachmentInfo.loadOp				= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		StencilAttachmentInfo.storeOp				= VK_ATTACHMENT_STORE_OP_STORE;
		//StencilAttachmentInfo.clearValue			= { 0.0f, 0.0f, 0.0f, 0.0f };

		RenderingInfo.sType							= VK_STRUCTURE_TYPE_RENDERING_INFO;
		RenderingInfo.pNext							= NULL;
		RenderingInfo.flags							= 0;
		RenderingInfo.renderArea					= aRenderArea;
		RenderingInfo.layerCount					= 1;
		RenderingInfo.viewMask						= 1;
		RenderingInfo.colorAttachmentCount			= ColorAttachmentInfo.size();
		RenderingInfo.pColorAttachments				= ColorAttachmentInfo.data();
		RenderingInfo.pDepthAttachment				= &DepthAttachmentInfo;
		RenderingInfo.pStencilAttachment			= &StencilAttachmentInfo;
		vkCmdBeginRendering(this->Handle, &RenderingInfo);
	}

	void command_buffer::end_rendering() {
		PFN_vkCmdEndRendering vkCmdEndRendering = (PFN_vkCmdEndRendering)this->Context->function_pointer("vkCmdEndRendering");
		vkCmdEndRendering(this->Handle);
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
