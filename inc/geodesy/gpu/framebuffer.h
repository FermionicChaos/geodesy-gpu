#pragma once
#ifndef GEODESY_GPU_FRAMEBUFFER_H
#define GEODESY_GPU_FRAMEBUFFER_H

#include "config.h"

#include "image.h"

namespace geodesy::gpu {

	// This class is used to interface actual resource attachements to a pipeline. Similar to descriptor sets.
	class framebuffer : public resource {
	public:

		std::vector<VkClearValue> ClearValue;
		VkFramebuffer Handle;

		framebuffer();
		framebuffer(std::shared_ptr<context> aContext, std::shared_ptr<pipeline> aPipeline, std::vector<std::shared_ptr<image>> aImageAttachements, std::array<unsigned int, 3> aResolution);
		framebuffer(std::shared_ptr<context> aContext, std::shared_ptr<pipeline> aPipeline, std::map<std::string, std::shared_ptr<image>> aImage, std::vector<std::string> aAttachmentSelection, std::array<unsigned int, 3> aResolution);
		~framebuffer();

	};

}

#endif // !GEODESY_GPU_FRAMEBUFFER_H