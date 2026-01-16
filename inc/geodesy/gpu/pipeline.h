#pragma once
#ifndef GEODESY_GPU_PIPELINE_H
#define GEODESY_GPU_PIPELINE_H

#include "config.h"

#include "device.h"
#include "resource.h"
#include "buffer.h"
#include "image.h"
#include "shader.h"
#include "descriptor.h"
#include "framebuffer.h"
#include "acceleration_structure.h"

/*
Flow Chart of Pipeline Creation.
Start:		     Vertex       Pixel
			[    Source       Source
Shader		[	    |			|
			[      AST         AST
					  \       / 
			[          Program
Rasterizer	[		  /       \
			[     SPIRV       SPIRV
			        |           |
			[     Module      Module
Device		[         \        / 
			[		   Pipeline
*/

namespace geodesy::gpu {

	class framebuffer;

	class pipeline : public std::enable_shared_from_this<pipeline>, public resource {
	public:

		enum stage : unsigned int {
			TOP_OF_PIPE = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
			DRAW_INDIRECT = VK_PIPELINE_STAGE_DRAW_INDIRECT_BIT,
			VERTEX_INPUT = VK_PIPELINE_STAGE_VERTEX_INPUT_BIT,
			VERTEX_SHADER = VK_PIPELINE_STAGE_VERTEX_SHADER_BIT,
			TESSELLATION_CONTROL_SHADER = VK_PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT,
			TESSELLATION_EVALUATION_SHADER = VK_PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT,
			GEOMETRY_SHADER = VK_PIPELINE_STAGE_GEOMETRY_SHADER_BIT,
			FRAGMENT_SHADER = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			EARLY_FRAGMENT_TESTS = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
			LATE_FRAGMENT_TESTS = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT,
			COLOR_ATTACHMENT_OUTPUT = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			COMPUTE_SHADER = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT,
			TRANSFER = VK_PIPELINE_STAGE_TRANSFER_BIT,
			BOTTOM_OF_PIPE = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
			HOST = VK_PIPELINE_STAGE_HOST_BIT,
			ALL_GRAPHICS = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT,
			ALL_COMMANDS = VK_PIPELINE_STAGE_ALL_COMMANDS_BIT,
			NONE = VK_PIPELINE_STAGE_NONE,
		};

		enum type : int {
			UNKNOWN = -1,
			RASTERIZER = VK_PIPELINE_BIND_POINT_GRAPHICS,
			COMPUTE = VK_PIPELINE_BIND_POINT_COMPUTE,
			RAY_TRACER = VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR,
		};

		enum input_rate {
			VERTEX = VK_VERTEX_INPUT_RATE_VERTEX,
			INSTANCE = VK_VERTEX_INPUT_RATE_INSTANCE,
		};

		struct create_info {
		public:

			// I think this interface data exists in all pipelines types.
			// Uniform metatdata
			std::vector<std::vector<VkDescriptorSetLayoutBinding>>							DescriptorSetLayoutBinding;			// Vulkan Spec Minimum Req: 4 Descriptor Sets
			std::map<std::pair<int, int>, const glslang::TObjectReflection*> 				DescriptorSetVariable;

			type																			BindPoint;
			std::vector<std::shared_ptr<shader>> 											Shader;								// 5 Stages For Rasterization Graphics (3 are Optional)
			std::shared_ptr<glslang::TProgram> 												Program;							// glslang linked program.
			std::vector<std::vector<unsigned int>> 											ByteCode; 							// SPIRV Byte Code for each Shader Stage.

			virtual ~create_info() = default;

			void generate_descriptor_set_layout_binding();

		};

		// Pre creation options for a rasterizer pipeline.
		struct rasterizer : public create_info {

			friend class pipeline;

			struct attribute {
				VkVertexInputAttributeDescription Description;
				const glslang::TObjectReflection* Variable;
			};

			struct attachment {
				VkAttachmentDescription Description;
				const glslang::TObjectReflection* Variable;
			};

			std::vector<VkVertexInputBindingDescription> VertexBufferBindingDescription;
			std::vector<attribute> VertexAttribute;

			std::vector<attachment> ColorAttachment;
			attachment DepthStencilAttachment;

			VkPrimitiveTopology PrimitiveTopology;
			float MinDepth;
			float MaxDepth;
			VkPolygonMode PolygonMode;
			VkCullModeFlags CullMode;
			VkFrontFace FrontFace;
			float LineWidth;
			bool DepthTestEnable;
			bool DepthWriteEnable;
			VkCompareOp DepthCompareOp;
			std::vector<VkPipelineColorBlendAttachmentState> AttachmentBlendingRules;

			rasterizer();
			rasterizer(std::vector<std::shared_ptr<shader>> aShaderList);

			void bind(uint32_t aBindingIndex, size_t aVertexStride, uint32_t aLocationIndex, size_t aVertexOffset, input_rate aInputRate = input_rate::VERTEX);

			void attach(uint32_t aAttachmentIndex, std::shared_ptr<image> aAttachmentImage, image::layout aImageLayout = image::layout::SHADER_READ_ONLY_OPTIMAL);
			void attach(uint32_t aAttachmentIndex, image::format aFormat, image::layout aImageLayout = image::layout::SHADER_READ_ONLY_OPTIMAL);

		};

		// Pre creation options for a raytracer pipeline.
		struct raytracer : public create_info {
			
			struct shader_group {
				std::shared_ptr<shader> GeneralShader;
				std::shared_ptr<shader> ClosestHitShader;
				std::shared_ptr<shader> AnyHitShader;
				std::shared_ptr<shader> IntersectionShader;
			};

			struct shader_binding_table {
				std::shared_ptr<buffer> Buffer;
				VkStridedDeviceAddressRegionKHR Raygen;
				VkStridedDeviceAddressRegionKHR Miss;
				VkStridedDeviceAddressRegionKHR Hit;
				VkStridedDeviceAddressRegionKHR Callable;
			};

			uint32_t MaxRecursionDepth;
			std::vector<shader_group> ShaderGroup;

			raytracer();
			raytracer(std::vector<shader_group> aShaderGroup, uint32_t aMaxRecursionDepth);

		};

		// Pre creation options for a compute pipeline.
		// Requires on a single compute shader.
		struct compute : public create_info {

			compute();
			compute(std::shared_ptr<shader> aComputeShader, std::array<unsigned int, 3> aThreadGroupCount = { 1, 1, 1 }, std::array<unsigned int, 3> aThreadGroupSize = { 1, 1, 1 });

		};

		static void barrier(
			command_buffer* aCommandBuffer,
			unsigned int aSrcStage, unsigned int aDstStage,
			unsigned int aSrcAccess, unsigned int aDstAccess
		);
		static void barrier(
			command_buffer* aCommandBuffer,
			unsigned int aSrcStage, unsigned int aDstStage,
			const std::vector<VkMemoryBarrier>& aMemoryBarrier = {},
			const std::vector<VkBufferMemoryBarrier>& aBufferBarrier = {},
			const std::vector<VkImageMemoryBarrier>& aImageBarrier = {}
		);

		std::shared_ptr<create_info> CreateInfo;

		std::vector<VkPipelineShaderStageCreateInfo> Stage;
		VkPipelineBindPoint BindPoint;
		VkPipelineLayout Layout;
		VkPipelineCache Cache;
		VkPipeline Handle;
		VkDescriptorPool DescriptorPool;
		std::vector<VkDescriptorSetLayout> DescriptorSetLayout;
		VkRenderPass RenderPass;
		raytracer::shader_binding_table ShaderBindingTable;

		pipeline();
		pipeline(std::shared_ptr<context> aContext, std::shared_ptr<rasterizer> aRasterizer, VkRenderPass aRenderPass = VK_NULL_HANDLE, uint32_t aSubpassIndex = 0);
		pipeline(std::shared_ptr<context> aContext, std::shared_ptr<raytracer> aRaytracer);
		pipeline(std::shared_ptr<context> aContext, std::shared_ptr<compute> aCompute);
		~pipeline();

		// Can be used for rasterization, raytracing, or compute.
		void bind(
			command_buffer* 											aCommandBuffer, 
			std::vector<std::shared_ptr<buffer>> 						aVertexBuffer = {}, 
			std::shared_ptr<buffer> 									aIndexBuffer = nullptr, 
			std::shared_ptr<descriptor::array> 							aDescriptorArray = nullptr
		);

		// Rasterization API
		void begin(command_buffer* aCommandBuffer, std::shared_ptr<framebuffer> aFrame, VkRect2D aRenderArea, VkSubpassContents aSubpassContents = VK_SUBPASS_CONTENTS_INLINE);
		void rasterize(
			command_buffer* 				 							aCommandBuffer,
			std::shared_ptr<framebuffer> 								aFramebuffer,
			std::array<unsigned int, 3> 								aResolution,
			std::vector<std::shared_ptr<buffer>> 						aVertexBuffer = {},
			std::shared_ptr<buffer> 									aIndexBuffer = nullptr,
			std::shared_ptr<descriptor::array> 							aDescriptorArray = nullptr
		);
		void end(command_buffer* aCommandBuffer);
		// Raytracing API
		void raytrace(
			command_buffer* 				 							aCommandBuffer,
			std::array<unsigned int, 3> 								aResolution,
			std::shared_ptr<descriptor::array> 							aDescriptorArray
		);
		// Compute API
		void dispatch(
			command_buffer* 				 							aCommandBuffer,
			std::array<unsigned int, 3> 								aThreadGroupCount,
			std::shared_ptr<descriptor::array> 							aDescriptorArray
		);

		// Immediate Mode Execution (Not Recommended For Performance Critical Code)
		// Rasterization API
		VkResult rasterize(
			std::shared_ptr<framebuffer> 								aFramebuffer,
			std::array<unsigned int, 3> 								aResolution,
			std::vector<std::shared_ptr<buffer>> 						aVertexBuffer = {},
			std::shared_ptr<buffer> 									aIndexBuffer = nullptr,
			std::shared_ptr<descriptor::array> 							aDescriptorArray = nullptr
		);
		VkResult rasterize(
			std::vector<std::shared_ptr<image>> 						aImage,
			std::array<unsigned int, 3> 								aResolution,
			std::vector<std::shared_ptr<buffer>> 						aVertexBuffer = {},
			std::shared_ptr<buffer> 									aIndexBuffer = nullptr,
			std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding = {}
		);
		// Raytracing API
		VkResult raytrace(
			std::shared_ptr<image> 										aOutputImage,
			std::shared_ptr<acceleration_structure> 					aTLAS,
			std::array<unsigned int, 3> 								aResolution,
			std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding = {}
		);
		// Compute API
		VkResult dispatch(
			std::array<unsigned int, 3> 								aThreadGroupCount,
			std::shared_ptr<descriptor::array> 							aDescriptorArray
		);
		VkResult dispatch(
			std::array<unsigned int, 3> 								aThreadGroupCount,
			std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding = {}
		);

		std::vector<VkDescriptorPoolSize> descriptor_pool_sizes() const;
		std::map<VkDescriptorType, uint32_t> descriptor_type_count() const;
		std::vector<std::vector<VkDescriptorSetLayoutBinding>> descriptor_set_layout_binding() const;

	private:

		VkResult shader_stage_create(std::shared_ptr<create_info> aCreateInfo);
		VkResult create_pipeline_layout(std::vector<std::vector<VkDescriptorSetLayoutBinding>> aDescriptorSetLayoutBinding);

	};

}

#endif // !GEODESY_GPU_PIPELINE_H
