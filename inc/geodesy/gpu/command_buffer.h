#ifndef GEODESY_GPU_COMMAND_BUFFER_H
#define GEODESY_GPU_COMMAND_BUFFER_H

#include "config.h"

#include "resource.h"

namespace geodesy::gpu {

	class command_buffer : public resource {
	public:
	
		std::shared_ptr<command_pool> 		CommandPool;

		VkCommandBuffer 					Handle;

		command_buffer();
		command_buffer(std::shared_ptr<context> aContext, std::shared_ptr<command_pool> aCommandPool, VkCommandBuffer aHandle);
		command_buffer(std::shared_ptr<context> aContext, std::shared_ptr<command_pool> aCommandPool, VkCommandBufferLevel aLevel = VK_COMMAND_BUFFER_LEVEL_PRIMARY);
		virtual ~command_buffer();

		VkResult begin();
		VkResult end();

		void bind_vertex_buffers(VkCommandBuffer aCommandBuffer, std::vector<VkBuffer> aBufferList, const VkDeviceSize* aOffset = NULL);
		void bind_index_buffer(VkCommandBuffer aCommandBuffer, VkBuffer aBufferHandle, VkIndexType aIndexType);
		void bind_descriptor_sets(VkCommandBuffer aCommandBuffer, VkPipelineBindPoint aPipelineBindPoint, VkPipelineLayout aPipelineLayout, std::vector<VkDescriptorSet> aDescriptorSetList, std::vector<uint32_t> aDynamicOffsetList = std::vector<uint32_t>(0));
		void bind_pipeline(VkCommandBuffer aCommandBuffer, VkPipelineBindPoint aPipelineBindPoint, VkPipeline aPipelineHandle);
		void draw_indexed(VkCommandBuffer aCommandBuffer, uint32_t aIndexCount, uint32_t aInstanceCount = 1, uint32_t aFirstIndex = 0, uint32_t aVertexOffset = 0, uint32_t aFirstInstance = 0);

	};
	
}

#endif // !GEODESY_GPU_COMMAND_BUFFER_H
