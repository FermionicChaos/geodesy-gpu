#include <geodesy/gpu/framebuffer.h>
#include <geodesy/gpu/pipeline.h>
#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	framebuffer::framebuffer(std::shared_ptr<context> aContext, std::shared_ptr<pipeline> aPipeline, std::vector<std::shared_ptr<image>> aImageAttachements, std::array<unsigned int, 3> aResolution) {
		PFN_vkCreateFramebuffer vkCreateFramebuffer = (PFN_vkCreateFramebuffer)aContext->function_pointer("vkCreateFramebuffer");
		this->Context = aContext;
		this->ClearValue = std::vector<VkClearValue>(aImageAttachements.size());
		for (size_t i = 0; i < aImageAttachements.size(); i++) {
			this->ClearValue[i].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		}
		std::vector<VkImageView> Attachment(aImageAttachements.size());
		for (size_t i = 0; i < aImageAttachements.size(); i++) {
			Attachment[i] = aImageAttachements[i]->View;
		}
		VkResult Result = VK_SUCCESS;
		VkFramebufferCreateInfo FBCI{};
		FBCI.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FBCI.pNext				= NULL;
		FBCI.flags				= 0;
		FBCI.renderPass			= aPipeline->RenderPass;
		FBCI.attachmentCount	= Attachment.size();
		FBCI.pAttachments		= Attachment.data();
		FBCI.width				= aResolution[0];
		FBCI.height				= aResolution[1];
		FBCI.layers				= 1;
		Result = vkCreateFramebuffer(aContext->Handle, &FBCI, NULL, &this->Handle);
	}

	framebuffer::framebuffer(std::shared_ptr<context> aContext, std::shared_ptr<pipeline> aPipeline, std::map<std::string, std::shared_ptr<image>> aImage, std::vector<std::string> aAttachmentSelection, std::array<unsigned int, 3> aResolution) {
		PFN_vkCreateFramebuffer vkCreateFramebuffer = (PFN_vkCreateFramebuffer)aContext->function_pointer("vkCreateFramebuffer");
		this->Context = aContext;
		this->ClearValue = std::vector<VkClearValue>(aAttachmentSelection.size());
		for (size_t i = 0; i < aAttachmentSelection.size(); i++) {
			this->ClearValue[i].color = { 0.0f, 0.0f, 0.0f, 1.0f };
		}
		std::vector<VkImageView> Attachment(aAttachmentSelection.size());
		for (size_t i = 0; i < aAttachmentSelection.size(); i++) {
			Attachment[i] = aImage[aAttachmentSelection[i]]->View;
		}
		VkResult Result = VK_SUCCESS;
		VkFramebufferCreateInfo FBCI{};
		FBCI.sType				= VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		FBCI.pNext				= NULL;
		FBCI.flags				= 0;
		FBCI.renderPass			= aPipeline->RenderPass;
		FBCI.attachmentCount	= Attachment.size();
		FBCI.pAttachments		= Attachment.data();
		FBCI.width				= aResolution[0];
		FBCI.height				= aResolution[1];
		FBCI.layers				= 1;
		Result = vkCreateFramebuffer(aContext->Handle, &FBCI, NULL, &this->Handle);
	}

	framebuffer::~framebuffer() {
		PFN_vkDestroyFramebuffer vkDestroyFramebuffer = (PFN_vkDestroyFramebuffer)this->Context->function_pointer("vkDestroyFramebuffer");
		vkDestroyFramebuffer(this->Context->Handle, this->Handle, NULL);
	}

}
