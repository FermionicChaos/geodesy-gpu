#include <geodesy/gpu/pipeline.h>
#include <geodesy/gpu/context.h>

#include "glslang_util.h"

namespace geodesy::gpu {

	void pipeline::create_info::generate_descriptor_set_layout_binding() {
		// This reduces code redundancy in DSLB for each pipeline type. Now all pipeline types
		// will extract uniform metadata in the same fashion. 

		// Get Uniform Blocks (uniform buffers).
		for (size_t i = 0; i < this->Program->getNumUniformBlocks(); i++) {
			VkDescriptorSetLayoutBinding DSLB{};
			const glslang::TObjectReflection& Variable = this->Program->getUniformBlock(i);
			const glslang::TType* Type = Variable.getType();
			const glslang::TArraySizes* ArraySize = Type->getArraySizes();
			size_t DescriptorCount = 0;
			if (ArraySize != NULL) {
				for (int j = 0; j < ArraySize->getNumDims(); j++) {
					DescriptorCount += ArraySize->getDimSize(j);
				}
			} else {
				DescriptorCount = 1;
			}

			int SetIndex = Type->getQualifier().layoutSet;
			int BindingIndex = Type->getQualifier().layoutBinding;
			std::pair<int, int> SetBinding = std::make_pair(SetIndex, BindingIndex);

			// Generate Bindings for Uniform Buffers.
			DSLB.binding = BindingIndex;
			switch(Type->getQualifier().storage) {
			case glslang::TStorageQualifier::EvqUniform:
				DSLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
				break;
			case glslang::TStorageQualifier::EvqBuffer:
				DSLB.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
				break;
			}
			DSLB.descriptorCount = DescriptorCount;
			DSLB.stageFlags = glslang_shader_stage_to_vulkan(Variable.stages);
			DSLB.pImmutableSamplers = NULL;

			// Resize DescriptorSetLayoutBinding if SetIndex does not exist.
			if (SetIndex >= this->DescriptorSetLayoutBinding.size()) {
				this->DescriptorSetLayoutBinding.resize(SetIndex + 1);
			}

			this->DescriptorSetLayoutBinding[SetIndex].push_back(DSLB);
			this->DescriptorSetVariable[SetBinding] = &Variable;
		}
		
		// Acquires uniform images, samplers, acceleration structures.
		for (int i = 0; i < this->Program->getNumUniformVariables(); i++) {
			const glslang::TObjectReflection& Variable = this->Program->getUniform(i);
			const glslang::TType* Type = Variable.getType();				
			const glslang::TArraySizes* ArraySize = Type->getArraySizes();
			size_t DescriptorCount = 0;
			if (ArraySize != NULL) {
				for (int j = 0; j < ArraySize->getNumDims(); j++) {
					DescriptorCount += ArraySize->getDimSize(j);
				}
			} else {
				DescriptorCount = 1;
			}

			int SetIndex = Type->getQualifier().layoutSet;
			int BindingIndex = Type->getQualifier().layoutBinding;
			std::pair<int, int> SetBinding = std::make_pair(SetIndex, BindingIndex);

			VkDescriptorSetLayoutBinding DSLB{};
			DSLB.binding = BindingIndex;
			switch(Type->getBasicType()){
			case glslang::TBasicType::EbtSampler:
				if (Type->getSampler().isImage()) {
					// Storage Image (read/write image)
					DSLB.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
				} else if (Type->getSampler().isBuffer()) {
					// Texel buffer - either uniform (read-only) or storage (read-write)
					if (Type->getSampler().isImageClass()) {
						// Storage texel buffer (read-write)
						DSLB.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER;
					} else {
						// Uniform texel buffer (read-only)
						DSLB.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER;
					}
				} else if (Type->getSampler().isPureSampler()) {
					// Standalone sampler
					DSLB.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
				} else {
					// Regular Combined Image Sampler
					DSLB.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
				}
				break;
			case glslang::TBasicType::EbtAccStruct:
				// Acceleration Structure (top-level or bottom-level)
				DSLB.descriptorType = VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR;
				break;
			default:
				// TODO: Add other variables later.
				// Skip other types we don't care for.
				continue;
			}
			DSLB.descriptorCount = DescriptorCount;
			DSLB.stageFlags = glslang_shader_stage_to_vulkan(Variable.stages);
			DSLB.pImmutableSamplers = NULL;

			// Resize DescriptorSetLayoutBinding if SetIndex does not exist
			if (SetIndex >= this->DescriptorSetLayoutBinding.size()) {
				this->DescriptorSetLayoutBinding.resize(SetIndex + 1);
			}

			this->DescriptorSetLayoutBinding[SetIndex].push_back(DSLB);
			this->DescriptorSetVariable[SetBinding] = &Variable;
		}
	}

	pipeline::rasterizer::rasterizer() {
		this->BindPoint 											= type::RASTERIZER;
		this->PrimitiveTopology										= VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		this->MinDepth												= 0.0f;
		this->MaxDepth												= 1.0f;
		this->PolygonMode											= VK_POLYGON_MODE_FILL;
		this->CullMode												= VK_CULL_MODE_NONE;
		this->FrontFace												= VK_FRONT_FACE_COUNTER_CLOCKWISE;
		this->LineWidth												= 1.0f;
		this->DepthTestEnable										= false;
		this->DepthWriteEnable										= true;
		this->DepthCompareOp										= VK_COMPARE_OP_LESS;
		this->DepthStencilAttachment.Description.flags 				= 0;
		this->DepthStencilAttachment.Description.format 			= VK_FORMAT_UNDEFINED;
		this->DepthStencilAttachment.Description.samples 			= VK_SAMPLE_COUNT_1_BIT;
		this->DepthStencilAttachment.Description.loadOp 			= VK_ATTACHMENT_LOAD_OP_LOAD;
		this->DepthStencilAttachment.Description.storeOp 			= VK_ATTACHMENT_STORE_OP_STORE;
		this->DepthStencilAttachment.Description.stencilLoadOp 		= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		this->DepthStencilAttachment.Description.stencilStoreOp 	= VK_ATTACHMENT_STORE_OP_DONT_CARE;
		this->DepthStencilAttachment.Description.initialLayout 		= VK_IMAGE_LAYOUT_UNDEFINED;
		this->DepthStencilAttachment.Description.finalLayout 		= VK_IMAGE_LAYOUT_UNDEFINED;
	}

	pipeline::rasterizer::rasterizer(std::vector<std::shared_ptr<shader>> aShaderList) : rasterizer() {
		bool Success = true;

		// Load shaders.
		this->Shader = aShaderList;

		// Link Shader Stages.
		if (Success) {
			EShMessages Message = (EShMessages)(
				EShMessages::EShMsgAST |
				EShMessages::EShMsgSpvRules |
				EShMessages::EShMsgVulkanRules |
				EShMessages::EShMsgDebugInfo |
				EShMessages::EShMsgBuiltinSymbolTable
			);

			// Link various shader stages together.
			this->Program = std::make_shared<glslang::TProgram>();
			for (std::shared_ptr<shader> Shd : Shader) {
				this->Program->addShader(Shd->Handle.get());
			}

			// Link Shader Stages
			Success = this->Program->link(Message);

			// Check if Link was successful
			if (!Success) {
				throw std::runtime_error("Failed to link shader stages for rasterization pipeline: " + std::string(this->Program->getInfoLog()));
			}
		}
		else {
			// No shaders provided.
			Success = false;
		}

		// Build API reflection.
		if (Success) {
			// Generates API Reflection.
			this->Program->buildReflection(EShReflectionAllIOVariables);

			// -------------------- START -------------------- //

			// Get Vertex Attribute Inputs
			this->VertexAttribute = std::vector<attribute>(this->Program->getNumPipeInputs());
			for (size_t i = 0; i < this->VertexAttribute.size(); i++) {
				const glslang::TObjectReflection& Variable 				= this->Program->getPipeInput(i);
				const glslang::TType* Type 								= Variable.getType();

				int Location 											= Type->getQualifier().layoutLocation;

				if (Location >= this->VertexAttribute.size()) {
					// Throw runtime error, locations must be continguous.
					throw std::runtime_error("Vertex Attribute Locations must be contiguous.");
				}

				this->VertexAttribute[Location].Variable 				= &Variable;

				// Load Attribute Descriptions
				this->VertexAttribute[Location].Description.location 	= Location;
				this->VertexAttribute[Location].Description.binding 	= 0;
				this->VertexAttribute[Location].Description.format 		= image::glsl_to_format(Variable);
				this->VertexAttribute[Location].Description.offset 		= 0;
			}

			// Get Framebuffer Attachment Outputs
			this->ColorAttachment = std::vector<struct attachment>(this->Program->getNumPipeOutputs());
			for (size_t i = 0; i < this->ColorAttachment.size(); i++) {
				const glslang::TObjectReflection& Variable 						= this->Program->getPipeOutput(i);
				const glslang::TType* Type 										= Variable.getType();

				int Location 													= Type->getQualifier().layoutLocation;

				if (Location >= this->ColorAttachment.size()) {
					// Throw runtime error, locations must be continguous.
					throw std::runtime_error("Color Attachment Locations must be contiguous.");
				}

				this->ColorAttachment[Location].Variable 						= &Variable;
				this->ColorAttachment[Location].Description.flags				= 0;
				this->ColorAttachment[Location].Description.format				= image::glsl_to_format(Variable);
				this->ColorAttachment[Location].Description.samples				= VK_SAMPLE_COUNT_1_BIT;
				this->ColorAttachment[Location].Description.loadOp				= VK_ATTACHMENT_LOAD_OP_LOAD;
				this->ColorAttachment[Location].Description.storeOp				= VK_ATTACHMENT_STORE_OP_STORE;
				this->ColorAttachment[Location].Description.stencilLoadOp		= VK_ATTACHMENT_LOAD_OP_DONT_CARE;
				this->ColorAttachment[Location].Description.stencilStoreOp		= VK_ATTACHMENT_STORE_OP_DONT_CARE;
				this->ColorAttachment[Location].Description.initialLayout		= VK_IMAGE_LAYOUT_UNDEFINED;
				this->ColorAttachment[Location].Description.finalLayout			= VK_IMAGE_LAYOUT_UNDEFINED;
			}

			// Generates Descriptor Set Layout Bindings.
			this->generate_descriptor_set_layout_binding();
			
		}
		else {
			// Linking failed.
			Success = false;
		}

		// Setup default blending rules for each color attachment.
		this->AttachmentBlendingRules = std::vector<VkPipelineColorBlendAttachmentState>(this->ColorAttachment.size());
		for (size_t i = 0; i < AttachmentBlendingRules.size(); i++) {
			AttachmentBlendingRules[i].blendEnable					= VK_FALSE;
			AttachmentBlendingRules[i].srcColorBlendFactor			= VK_BLEND_FACTOR_SRC_ALPHA;
			AttachmentBlendingRules[i].dstColorBlendFactor			= VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
			AttachmentBlendingRules[i].colorBlendOp					= VK_BLEND_OP_ADD;
			AttachmentBlendingRules[i].srcAlphaBlendFactor			= VK_BLEND_FACTOR_ONE;
			AttachmentBlendingRules[i].dstAlphaBlendFactor			= VK_BLEND_FACTOR_ZERO;
			AttachmentBlendingRules[i].alphaBlendOp					= VK_BLEND_OP_ADD;
			AttachmentBlendingRules[i].colorWriteMask				= VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		}

		// Generate SPIRV code.
		if (Success) {
			glslang::SpvOptions Option;
			spv::SpvBuildLogger Logger;
			this->ByteCode = std::vector<std::vector<unsigned int>>(this->Shader.size());
			for (size_t i = 0; i < this->Shader.size(); i++) {
				glslang::GlslangToSpv(*this->Program->getIntermediate(this->Shader[i]->Handle->getStage()), this->ByteCode[i], &Logger, &Option);
			}
		}
		else {
			// failed to gather meta data.
			Success = false;
		}
	}

	void pipeline::rasterizer::bind(uint32_t aBindingIndex, size_t aVertexStride, uint32_t aLocationIndex, size_t aVertexOffset, input_rate aInputRate) {
		// Check if binding already exists in set.
		bool ExistsInSet = false;
		for (const VkVertexInputBindingDescription& Buffer : this->VertexBufferBindingDescription) {
			ExistsInSet |= (aBindingIndex == Buffer.binding);
		}

		if (!ExistsInSet) {
			// Does not exist in set, add it.
			VkVertexInputBindingDescription Buffer{};
			Buffer.binding		= aBindingIndex;
			Buffer.stride		= aVertexStride;
			Buffer.inputRate	= (VkVertexInputRate)aInputRate;
			this->VertexBufferBindingDescription.push_back(Buffer);
		}
		else {
			// Exists in set, overwrite it.
			for (VkVertexInputBindingDescription& Buffer : this->VertexBufferBindingDescription) {
				if (aBindingIndex == Buffer.binding) {
					Buffer.binding		= aBindingIndex;
					Buffer.stride		= aVertexStride;
					Buffer.inputRate	= (VkVertexInputRate)aInputRate;
				}
			}
		}

		for (attribute& Attribute : this->VertexAttribute) {
			if (aLocationIndex == Attribute.Description.location) {
				Attribute.Description.location			= aLocationIndex;
				Attribute.Description.binding			= aBindingIndex;
				//Attribute.Description.format		; // Gets set in Meta Data Section.
				Attribute.Description.offset			= aVertexOffset;
				break;
			}
		}
	}

	void pipeline::rasterizer::attach(uint32_t aAttachmentIndex, std::shared_ptr<image> aAttachmentImage, image::layout aImageLayout) {
		// TODO: Maybe check that image sample count matches rasterizer sample count?
		this->attach(aAttachmentIndex, (image::format)aAttachmentImage->CreateInfo.format, aImageLayout);
	}

	void pipeline::rasterizer::attach(uint32_t aAttachmentIndex, image::format aFormat, image::layout aImageLayout) {
		if (aAttachmentIndex < this->ColorAttachment.size()) {
			this->ColorAttachment[aAttachmentIndex].Description.format			= (VkFormat)aFormat;
			this->ColorAttachment[aAttachmentIndex].Description.samples			= VK_SAMPLE_COUNT_1_BIT;
			this->ColorAttachment[aAttachmentIndex].Description.initialLayout	= (VkImageLayout)aImageLayout;
			this->ColorAttachment[aAttachmentIndex].Description.finalLayout		= (VkImageLayout)aImageLayout;
		}
		else if (aAttachmentIndex == this->ColorAttachment.size()) {
			this->DepthStencilAttachment.Description.format						= (VkFormat)aFormat;
			this->DepthStencilAttachment.Description.samples					= VK_SAMPLE_COUNT_1_BIT;
			this->DepthStencilAttachment.Description.initialLayout				= (VkImageLayout)aImageLayout;
			this->DepthStencilAttachment.Description.finalLayout				= (VkImageLayout)aImageLayout;
		}
	}

	pipeline::raytracer::raytracer() {
		this->MaxRecursionDepth = 0;
	}

	pipeline::raytracer::raytracer(std::vector<shader_group> aShaderGroup, uint32_t aMaxRecursionDepth) : raytracer() {
		bool Success = true;
		this->ShaderGroup = aShaderGroup;
		this->MaxRecursionDepth = aMaxRecursionDepth;
		
		// Now convert shader groups from pointers into std::vector<VkRayTracingShaderGroupCreateInfoKHR> & std::vector<std::shared_ptr<shader>>.
		{
			std::set<std::shared_ptr<shader>> LinearizedShaderSet;
			for (size_t i = 0; i < this->ShaderGroup.size(); i++) {
				std::vector<std::shared_ptr<shader>> ShaderList = { this->ShaderGroup[i].GeneralShader, this->ShaderGroup[i].ClosestHitShader, this->ShaderGroup[i].AnyHitShader, this->ShaderGroup[i].IntersectionShader };
				// Check if any of the shaders in ShaderList exist in this->Shader.
				for (size_t j = 0; j < ShaderList.size(); j++) {
					if (ShaderList[j] != nullptr) {
						LinearizedShaderSet.insert(ShaderList[j]);
					}
				}
			}
	
			// Convert into an std::vector<std::shared_ptr<shader>>.
			this->Shader = std::vector<std::shared_ptr<shader>>(LinearizedShaderSet.begin(), LinearizedShaderSet.end());
		}

		// Compile shaders.
		// Link Shader Stages.
		if (Success) {
			EShMessages Message = (EShMessages)(
				EShMessages::EShMsgAST |
				EShMessages::EShMsgSpvRules |
				EShMessages::EShMsgVulkanRules |
				EShMessages::EShMsgDebugInfo |
				EShMessages::EShMsgBuiltinSymbolTable
			);

			// Link various shader stages together.
			this->Program = std::make_shared<glslang::TProgram>();
			for (std::shared_ptr<shader> Shd : Shader) {
				this->Program->addShader(Shd->Handle.get());
			}

			// Link Shader Stages
			Success = this->Program->link(Message);

			// Check if Link was successful
			if (!Success) {
				throw std::runtime_error("Failed to link shader stages for ray tracing pipeline: " + std::string(this->Program->getInfoLog()));
			}
		}

		// TODO: Build and acquire reflection variables.
		if (Success) {
			this->Program->buildReflection(EShReflectionAllIOVariables);

			// Generates Descriptor Set Layout Bindings.
			this->generate_descriptor_set_layout_binding();
		
		}

		// Generate SPIRV code.
		if (Success) {
			glslang::SpvOptions Option;
			spv::SpvBuildLogger Logger;
			this->ByteCode = std::vector<std::vector<unsigned int>>(this->Shader.size());
			for (size_t i = 0; i < this->Shader.size(); i++) {
				glslang::GlslangToSpv(*this->Program->getIntermediate(this->Shader[i]->Handle->getStage()), this->ByteCode[i], &Logger, &Option);
			}
		}
	}

	pipeline::compute::compute() {
		this->BindPoint = pipeline::type::COMPUTE;
	}

	pipeline::compute::compute(std::shared_ptr<shader> aComputeShader, std::array<unsigned int, 3> aThreadGroupCount, std::array<unsigned int, 3> aThreadGroupSize) : compute() {

		// Compute Pipeline has only one shader stage.
		this->Shader = { aComputeShader };

		// Go ahead and link the shader and build reflection.
		EShMessages Message = (EShMessages)(
			EShMessages::EShMsgAST |
			EShMessages::EShMsgSpvRules |
			EShMessages::EShMsgVulkanRules |
			EShMessages::EShMsgDebugInfo |
			EShMessages::EShMsgBuiltinSymbolTable
		);

		// Link various shader stages together.
		this->Program = std::make_shared<glslang::TProgram>();
		for (std::shared_ptr<shader> Shd : Shader) {
			this->Program->addShader(Shd->Handle.get());
		}

		// Link Shader Stages
		bool Success = this->Program->link(Message);

		// Check if Link was successful
		if (!Success) {
			throw std::runtime_error("Failed to link shader stages for compute pipeline: " + std::string(this->Program->getInfoLog()));
		}

		// Build and acquire reflection variables.
		if (Success) {
			this->Program->buildReflection(EShReflectionAllIOVariables);

			// Generates Descriptor Set Layout Bindings.
			this->generate_descriptor_set_layout_binding();

		}

		// Generate SPIRV code.
		if (Success) {
			glslang::SpvOptions Option;
			spv::SpvBuildLogger Logger;
			this->ByteCode = std::vector<std::vector<unsigned int>>(this->Shader.size());
			for (size_t i = 0; i < this->Shader.size(); i++) {
				glslang::GlslangToSpv(*this->Program->getIntermediate(this->Shader[i]->Handle->getStage()), this->ByteCode[i], &Logger, &Option);
			}
		}
	}

	void pipeline::barrier(
		command_buffer* aCommandBuffer,
		unsigned int aSrcStage, unsigned int aDstStage,
		unsigned int aSrcAccess, unsigned int aDstAccess
	) {
		VkMemoryBarrier MemoryBarrier{};
		MemoryBarrier.sType				= VK_STRUCTURE_TYPE_MEMORY_BARRIER;
		MemoryBarrier.pNext				= NULL;
		MemoryBarrier.srcAccessMask		= aSrcAccess;
		MemoryBarrier.dstAccessMask		= aDstAccess;
		std::vector<VkMemoryBarrier> MemoryBarrierVector = { MemoryBarrier };
		pipeline::barrier(aCommandBuffer, aSrcStage, aDstStage, MemoryBarrierVector);
	}

	void pipeline::barrier(
		command_buffer* aCommandBuffer, 
		unsigned int aSrcStage, unsigned int aDstStage, 
		const std::vector<VkMemoryBarrier>& aMemoryBarrier, 
		const std::vector<VkBufferMemoryBarrier>& aBufferBarrier, 
		const std::vector<VkImageMemoryBarrier>& aImageBarrier
	) {
		PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)aCommandBuffer->Context->function_pointer("vkCmdPipelineBarrier");
		vkCmdPipelineBarrier(
			aCommandBuffer->Handle, 
			(VkPipelineStageFlags)aSrcStage, (VkPipelineStageFlags)aDstStage, 
			0, 
			aMemoryBarrier.size(), aMemoryBarrier.data(), 
			aBufferBarrier.size(), aBufferBarrier.data(), 
			aImageBarrier.size(), aImageBarrier.data()
		);
	}

	pipeline::pipeline() {
		this->Context 			= nullptr;
		this->Type				= resource::type::PIPELINE;
		this->BindPoint			= VK_PIPELINE_BIND_POINT_MAX_ENUM;
		this->Layout			= VK_NULL_HANDLE;
		this->Cache				= VK_NULL_HANDLE;
		this->Handle			= VK_NULL_HANDLE;
		this->DescriptorPool	= VK_NULL_HANDLE;
		this->RenderPass		= VK_NULL_HANDLE;
	}

	pipeline::pipeline(std::shared_ptr<context> aContext, std::shared_ptr<rasterizer> aRasterizer, VkRenderPass aRenderPass, uint32_t aSubpassIndex) : pipeline() {
		VkResult Result = VK_SUCCESS;
		PFN_vkCreateRenderPass vkCreateRenderPass = (PFN_vkCreateRenderPass)aContext->function_pointer("vkCreateRenderPass");
		PFN_vkCreateGraphicsPipelines vkCreateGraphicsPipelines = (PFN_vkCreateGraphicsPipelines)aContext->function_pointer("vkCreateGraphicsPipelines");		

		this->CreateInfo 	= aRasterizer;
		this->Context		= aContext;
		this->BindPoint		= VK_PIPELINE_BIND_POINT_GRAPHICS;

		// Create Render Pass.
		{
			// Attachments
			std::vector<VkAttachmentDescription> AttachmentDescription(aRasterizer->ColorAttachment.size());

			// References
			std::vector<VkAttachmentReference> ColorAttachmentReference(aRasterizer->ColorAttachment.size());
			VkAttachmentReference DepthAttachmentReference{};
			VkAttachmentReference StencilAttachmentReference{};

			// Load all Color Attachments.
			for (size_t i = 0; i < aRasterizer->ColorAttachment.size(); i++) {
				AttachmentDescription[i] = aRasterizer->ColorAttachment[i].Description;
			}
			
			// If depth stencil format is not undefined, add to attachment description.
			if (aRasterizer->DepthStencilAttachment.Description.format != VK_FORMAT_UNDEFINED) {
				DepthAttachmentReference = { (uint32_t)AttachmentDescription.size(), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
				AttachmentDescription.push_back(aRasterizer->DepthStencilAttachment.Description);
			}

			// Convert to references. 
			for (size_t i = 0; i < aRasterizer->ColorAttachment.size(); i++) {
				ColorAttachmentReference[i].attachment	= i;
				ColorAttachmentReference[i].layout		= VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}

			// Subpass Description
			std::vector<VkSubpassDescription> SubpassDescription(1);
			SubpassDescription[0].flags							= 0;
			SubpassDescription[0].pipelineBindPoint				= VK_PIPELINE_BIND_POINT_GRAPHICS;
			SubpassDescription[0].inputAttachmentCount			= 0;
			SubpassDescription[0].pInputAttachments				= NULL;
			SubpassDescription[0].colorAttachmentCount			= ColorAttachmentReference.size();
			SubpassDescription[0].pColorAttachments				= ColorAttachmentReference.data();
			SubpassDescription[0].pResolveAttachments			= NULL;
			if (aRasterizer->DepthStencilAttachment.Description.format != VK_FORMAT_UNDEFINED) {
				SubpassDescription[0].pDepthStencilAttachment		= &DepthAttachmentReference;
			}
			else {
				SubpassDescription[0].pDepthStencilAttachment		= NULL;
			}

			// Subpass Dependency
			std::vector<VkSubpassDependency> SubpassDependency(1);
			SubpassDependency[0].srcSubpass						= VK_SUBPASS_EXTERNAL;
			SubpassDependency[0].dstSubpass						= 0;
			SubpassDependency[0].srcStageMask					= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			SubpassDependency[0].dstStageMask					= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
			SubpassDependency[0].srcAccessMask					= 0;
			SubpassDependency[0].dstAccessMask					= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
			SubpassDependency[0].dependencyFlags				= 0;
			
			VkRenderPassCreateInfo RPCI{};
			RPCI.sType											= VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
			RPCI.pNext											= NULL;
			RPCI.flags											= 0;
			RPCI.attachmentCount								= AttachmentDescription.size();
			RPCI.pAttachments									= AttachmentDescription.data();
			RPCI.subpassCount									= SubpassDescription.size();
			RPCI.pSubpasses										= SubpassDescription.data();
			RPCI.dependencyCount								= SubpassDependency.size();
			RPCI.pDependencies									= SubpassDependency.data();

			Result = vkCreateRenderPass(aContext->Handle, &RPCI, NULL, &this->RenderPass);
		}

		// Create Respective Shader Modules.
		Result = this->shader_stage_create(aRasterizer);

		// Generate Descriptor Set Layouts from Meta Data gathered from shaders.
		Result = this->create_pipeline_layout(aRasterizer->DescriptorSetLayoutBinding);

		// Create Pipeline
		if (Result == VK_SUCCESS) {
			bool TesselationControlShaderExists = false;
			bool TesselationEvaluationShaderExists = false;
			for (std::shared_ptr<shader> Shdr : aRasterizer->Shader) {
				if (Shdr->Stage == shader::stage::TESSELLATION_CONTROL) {
					TesselationControlShaderExists = true;
				}
				if (Shdr->Stage == shader::stage::TESSELLATION_CONTROL) {
					TesselationEvaluationShaderExists = true;
				}
			}

			// Load attributes from aRasterizer->
			std::vector<VkVertexInputAttributeDescription> VertexAttributeDescription;
			for (rasterizer::attribute& Attribute : aRasterizer->VertexAttribute) {
				VertexAttributeDescription.push_back(Attribute.Description);
			}

			// Loading Vertex Binding & Attribute Data
			VkPipelineVertexInputStateCreateInfo Input{};
			Input.sType													= VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
			Input.pNext													= NULL;
			Input.flags													= 0;
			Input.vertexBindingDescriptionCount							= aRasterizer->VertexBufferBindingDescription.size();
			Input.pVertexBindingDescriptions							= aRasterizer->VertexBufferBindingDescription.data();
			Input.vertexAttributeDescriptionCount						= VertexAttributeDescription.size();
			Input.pVertexAttributeDescriptions							= VertexAttributeDescription.data();

			// Vulkan Objects
			VkPipelineInputAssemblyStateCreateInfo InputAssembly = {};
			VkPipelineTessellationStateCreateInfo Tesselation = {};
			VkPipelineViewportStateCreateInfo Viewport = {};
			VkPipelineRasterizationStateCreateInfo Rasterizer = {};
			VkPipelineMultisampleStateCreateInfo Multisample = {};
			VkPipelineDepthStencilStateCreateInfo DepthStencil = {};
			VkPipelineColorBlendStateCreateInfo	ColorBlend = {};
			VkPipelineDynamicStateCreateInfo DynamicState = {};

			InputAssembly.sType 							= VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
			InputAssembly.pNext 							= NULL;
			InputAssembly.flags 							= 0;
			InputAssembly.topology 							= aRasterizer->PrimitiveTopology;
			InputAssembly.primitiveRestartEnable 			= VK_FALSE;

			Tesselation.sType 								= VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
			Tesselation.pNext 								= NULL;
			Tesselation.flags 								= 0;

			// Both are stupid options, just tie to resolution.
			Viewport.sType 									= VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
			Viewport.pNext 									= NULL;
			Viewport.flags 									= 0;
			Viewport.viewportCount 							= 1;
			Viewport.pViewports 							= NULL;
			Viewport.scissorCount 							= 1;
			Viewport.pScissors 								= NULL;

			// Depth Clamp seems stupid, don't care about it.
			Rasterizer.sType 								= VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
			Rasterizer.pNext 								= NULL;
			Rasterizer.flags 								= 0;
			Rasterizer.depthClampEnable 					= VK_FALSE;
			Rasterizer.rasterizerDiscardEnable 				= VK_FALSE;
			Rasterizer.polygonMode 							= aRasterizer->PolygonMode;
			Rasterizer.cullMode 							= aRasterizer->CullMode;
			Rasterizer.frontFace 							= aRasterizer->FrontFace;
			Rasterizer.depthBiasEnable 						= VK_FALSE;
			Rasterizer.depthBiasConstantFactor 				= 0.0f;
			Rasterizer.depthBiasClamp 						= 0.0f;
			Rasterizer.depthBiasSlopeFactor 				= 0.0f;
			Rasterizer.lineWidth 							= aRasterizer->LineWidth;

			// Disabled By Default
			Multisample.sType 								= VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
			Multisample.pNext 								= NULL;
			Multisample.flags 								= 0;
			Multisample.rasterizationSamples				= VK_SAMPLE_COUNT_1_BIT;
			Multisample.sampleShadingEnable 				= VK_FALSE;
			Multisample.minSampleShading 					= 1.0f;
			Multisample.pSampleMask 						= NULL;
			Multisample.alphaToCoverageEnable 				= VK_FALSE;
			Multisample.alphaToOneEnable 					= VK_FALSE;

			// I don't care about stencil stuff, depth bounds is redundant, suppress.
			DepthStencil.sType 								= VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
			DepthStencil.pNext 								= NULL;
			DepthStencil.flags 								= 0;
			DepthStencil.depthTestEnable 					= aRasterizer->DepthTestEnable;
			DepthStencil.depthWriteEnable 					= aRasterizer->DepthWriteEnable;
			DepthStencil.depthCompareOp 					= aRasterizer->DepthCompareOp;
			DepthStencil.depthBoundsTestEnable 				= VK_FALSE;
			DepthStencil.stencilTestEnable 					= VK_FALSE;
			DepthStencil.front 								= {};
			DepthStencil.back 								= {};
			DepthStencil.minDepthBounds 					= 0.0f;
			DepthStencil.maxDepthBounds 					= 1.0f;

			ColorBlend.sType 								= VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
			ColorBlend.pNext 								= NULL;
			ColorBlend.flags 								= 0;
			ColorBlend.logicOpEnable 						= VK_FALSE;
			ColorBlend.logicOp 								= VK_LOGIC_OP_COPY;
			ColorBlend.attachmentCount 						= aRasterizer->AttachmentBlendingRules.size();
			ColorBlend.pAttachments 						= aRasterizer->AttachmentBlendingRules.data();
			ColorBlend.blendConstants[0] 					= 0.0f;
			ColorBlend.blendConstants[1] 					= 0.0f;
			ColorBlend.blendConstants[2] 					= 0.0f;
			ColorBlend.blendConstants[3] 					= 0.0f;

			// Default Enabled Dynamic State.
			std::vector<VkDynamicState> DynamicStates {
				VK_DYNAMIC_STATE_VIEWPORT,
				VK_DYNAMIC_STATE_SCISSOR
			};

			DynamicState.sType 								= VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
			DynamicState.pNext 								= NULL;
			DynamicState.flags 								= 0;
			DynamicState.dynamicStateCount 					= DynamicStates.size();
			DynamicState.pDynamicStates 					= DynamicStates.data();


			// Create Rasterizer Create Info Struct.
			VkGraphicsPipelineCreateInfo RasterizerCreateInfo {};
			RasterizerCreateInfo.sType							= VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
			RasterizerCreateInfo.pNext							= NULL;// &RenderingCreateInfo;
			RasterizerCreateInfo.flags							= 0;
			RasterizerCreateInfo.stageCount						= this->Stage.size();
			RasterizerCreateInfo.pStages						= this->Stage.data();
			RasterizerCreateInfo.pVertexInputState				= &Input;
			RasterizerCreateInfo.pInputAssemblyState			= &InputAssembly;
			if (TesselationControlShaderExists && TesselationEvaluationShaderExists) {
				RasterizerCreateInfo.pTessellationState				= &Tesselation;
			}
			else {
				RasterizerCreateInfo.pTessellationState				= NULL;
			}
			RasterizerCreateInfo.pViewportState					= &Viewport;
			RasterizerCreateInfo.pRasterizationState			= &Rasterizer;
			RasterizerCreateInfo.pMultisampleState				= &Multisample;
			if (aRasterizer->DepthStencilAttachment.Description.format != VK_FORMAT_UNDEFINED) {
				RasterizerCreateInfo.pDepthStencilState				= &DepthStencil;
			}
			else {
				RasterizerCreateInfo.pDepthStencilState				= NULL;
			}
			RasterizerCreateInfo.pColorBlendState				= &ColorBlend;
			RasterizerCreateInfo.pDynamicState					= &DynamicState;
			RasterizerCreateInfo.layout							= this->Layout;
			RasterizerCreateInfo.renderPass						= this->RenderPass;
			RasterizerCreateInfo.subpass						= 0;
			RasterizerCreateInfo.basePipelineHandle				= VK_NULL_HANDLE;
			RasterizerCreateInfo.basePipelineIndex				= 0;
			
			// Create Rasterization Pipeline.
			Result = vkCreateGraphicsPipelines(this->Context->Handle, this->Cache, 1, &RasterizerCreateInfo, NULL, &this->Handle);
		}
	}

	pipeline::pipeline(std::shared_ptr<context> aContext, std::shared_ptr<raytracer> aRaytracer) : pipeline() {
		VkResult Result = VK_SUCCESS;
		PFN_vkCreateRayTracingPipelinesKHR vkCreateRayTracingPipelinesKHR = (PFN_vkCreateRayTracingPipelinesKHR)aContext->function_pointer("vkCreateRayTracingPipelinesKHR");
		PFN_vkGetPhysicalDeviceProperties2 vkGetPhysicalDeviceProperties2 = (PFN_vkGetPhysicalDeviceProperties2)aContext->function_pointer("vkGetPhysicalDeviceProperties2");
		PFN_vkGetRayTracingShaderGroupHandlesKHR vkGetRayTracingShaderGroupHandlesKHR = (PFN_vkGetRayTracingShaderGroupHandlesKHR)aContext->function_pointer("vkGetRayTracingShaderGroupHandlesKHR");

		this->CreateInfo	= aRaytracer;
		this->Context		= aContext;
		this->BindPoint		= VK_PIPELINE_BIND_POINT_RAY_TRACING_KHR;

		// Generate GPU shader modules.
		Result = this->shader_stage_create(aRaytracer);

		// Generate Shader Group information.
		std::vector<VkRayTracingShaderGroupCreateInfoKHR> RSGCI(aRaytracer->ShaderGroup.size());
		for (size_t i = 0; i < aRaytracer->ShaderGroup.size(); i++) {
			RSGCI[i].sType									= VK_STRUCTURE_TYPE_RAY_TRACING_SHADER_GROUP_CREATE_INFO_KHR;
			RSGCI[i].pNext									= NULL;
			// Use inference to determine shader group type.
			if (aRaytracer->ShaderGroup[i].IntersectionShader != nullptr) {
				RSGCI[i].type								= VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR;
			}
			else if ((aRaytracer->ShaderGroup[i].AnyHitShader != nullptr) || (aRaytracer->ShaderGroup[i].ClosestHitShader != nullptr)) {
				RSGCI[i].type 								= VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR;
			}
			else {
				RSGCI[i].type								= VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR;
			}
			// TODO: Clean up this code later for index searching template code.
			{
				// find general shader location index.
				int Index = -1;
				{
					auto it = std::find(aRaytracer->Shader.begin(), aRaytracer->Shader.end(), aRaytracer->ShaderGroup[i].GeneralShader);
					if (it != aRaytracer->Shader.end()) {
						// Search for index of where this shader exists in aRaytracer->Shader.
						Index = (int)(it - aRaytracer->Shader.begin());
					}
				}
				if (Index >= 0) {
					RSGCI[i].generalShader							= Index;
				}
				else {
					RSGCI[i].generalShader							= VK_SHADER_UNUSED_KHR;
				}
			}
			{
				// find closest hit shader location index.
				int Index = -1;
				{
					auto it = std::find(aRaytracer->Shader.begin(), aRaytracer->Shader.end(), aRaytracer->ShaderGroup[i].ClosestHitShader);
					if (it != aRaytracer->Shader.end()) {
						// Search for index of where this shader exists in aRaytracer->Shader.
						Index = (int)(it - aRaytracer->Shader.begin());
					}
				}
				if (Index >= 0) {
					RSGCI[i].closestHitShader						= Index;
				}
				else {
					RSGCI[i].closestHitShader						= VK_SHADER_UNUSED_KHR;
				}
			}
			{
				// find any hit shader location index.
				int Index = -1;
				{
					auto it = std::find(aRaytracer->Shader.begin(), aRaytracer->Shader.end(), aRaytracer->ShaderGroup[i].AnyHitShader);
					if (it != aRaytracer->Shader.end()) {
						// Search for index of where this shader exists in aRaytracer->Shader.
						Index = (int)(it - aRaytracer->Shader.begin());
					}
				}
				if (Index >= 0) {
					RSGCI[i].anyHitShader							= Index;
				}
				else {
					RSGCI[i].anyHitShader							= VK_SHADER_UNUSED_KHR;
				}
			}
			{
				// Intersection Shader.
				int Index = -1;
				{
					auto it = std::find(aRaytracer->Shader.begin(), aRaytracer->Shader.end(), aRaytracer->ShaderGroup[i].IntersectionShader);
					if (it != aRaytracer->Shader.end()) {
						// Search for index of where this shader exists in aRaytracer->Shader.
						Index = (int)(it - aRaytracer->Shader.begin());
					}
				}
				if (Index >= 0) {
					RSGCI[i].intersectionShader						= Index;
				}
				else {
					RSGCI[i].intersectionShader						= VK_SHADER_UNUSED_KHR;
				}
			}			
		}

		// Create Pipeline Layout.
		Result = this->create_pipeline_layout(aRaytracer->DescriptorSetLayoutBinding);

		// Create Actual Ray Tracing Pipeline.
		if (Result == VK_SUCCESS) {
			VkRayTracingPipelineCreateInfoKHR RTPCI{};
			RTPCI.sType									= VK_STRUCTURE_TYPE_RAY_TRACING_PIPELINE_CREATE_INFO_KHR;
			RTPCI.pNext									= NULL;
			RTPCI.flags									= 0;
			RTPCI.stageCount							= this->Stage.size();
			RTPCI.pStages								= this->Stage.data();
			RTPCI.groupCount							= RSGCI.size();
			RTPCI.pGroups								= RSGCI.data();
			RTPCI.maxPipelineRayRecursionDepth			= aRaytracer->MaxRecursionDepth;
			RTPCI.pLibraryInfo							= NULL;
			RTPCI.pLibraryInterface						= NULL;
			RTPCI.pDynamicState							= NULL;
			RTPCI.layout								= this->Layout;
			RTPCI.basePipelineHandle					= VK_NULL_HANDLE;
			RTPCI.basePipelineIndex						= 0;
			
			// Requires loading function.
			Result = vkCreateRayTracingPipelinesKHR(aContext->Handle, VK_NULL_HANDLE, this->Cache, 1, &RTPCI, NULL, &this->Handle);
		}

		// Create Shader Binding Table.
		if (Result == VK_SUCCESS) {
			VkPhysicalDeviceRayTracingPipelinePropertiesKHR PDRTPP{};
			PDRTPP.sType								= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_RAY_TRACING_PIPELINE_PROPERTIES_KHR;
			PDRTPP.pNext								= NULL;

			VkPhysicalDeviceProperties2 PDP2{};
			PDP2.sType									= VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
			PDP2.pNext									= &PDRTPP;

			// Get Physical Device Properties.
			vkGetPhysicalDeviceProperties2(aContext->Device->Handle, &PDP2);

			// Get Shader Group Handle Size.
			uint32_t HandleSize						= PDRTPP.shaderGroupHandleSize;
			uint32_t HandleAlignment				= PDRTPP.shaderGroupHandleAlignment;
			uint32_t BaseAlignment 					= PDRTPP.shaderGroupBaseAlignment;
			uint32_t GroupCount						= (uint32_t)RSGCI.size();

			// Get Shader Group Handles.
			std::vector<uint8_t> ShaderGroupHandle(GroupCount * HandleSize);
			Result = vkGetRayTracingShaderGroupHandlesKHR(aContext->Handle, this->Handle, 0, GroupCount, HandleSize * GroupCount, ShaderGroupHandle.data());

			// Get Shader Region Counts
			uint32_t RayGenCount = 0, MissCount = 0, HitCount = 0, CallableCount = 0;
			for (size_t i = 0; i < RSGCI.size(); i++) {  // Use shader groups, not individual shaders
				switch (RSGCI[i].type) {
				case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
					// Need to determine if it's raygen, miss, or callable
					if (aRaytracer->ShaderGroup[i].GeneralShader) {
						switch (aRaytracer->ShaderGroup[i].GeneralShader->Stage) {
						case shader::stage::RAYGEN:
							RayGenCount++;
							break;
						case shader::stage::MISS:
							MissCount++;
							break;
						case shader::stage::CALLABLE:
							CallableCount++;
							break;
						default:
							break;
						}
					}
					break;
				case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
				case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR:
					HitCount++;
					break;
				default:
					break;
				}
			}
			
			// TODO: Maybe generalize to alignment function general?
			auto align_up = [](uint32_t aValue, uint32_t aAlignment) -> uint32_t {
				return (aValue + aAlignment - 1) & ~(aAlignment - 1);
			};
			
			// Calculate Region Sizes based on alignment.
			uint32_t HandleSizeAligned 		= align_up(HandleSize, HandleAlignment);
			// For raygen: size must equal stride (only one entry), so don't align to BaseAlignment
			uint32_t RayGenSize 			= RayGenCount > 0 ? HandleSizeAligned : 0;
			uint32_t MissSize 				= align_up(MissCount * HandleSizeAligned, BaseAlignment);
			uint32_t HitSize 				= align_up(HitCount * HandleSizeAligned, BaseAlignment);
			uint32_t CallableSize 			= align_up(CallableCount * HandleSizeAligned, BaseAlignment);
			
			// Calculate offsets with proper BaseAlignment for each region
			uint32_t RayGenOffset = 0;
			uint32_t MissOffset = align_up(RayGenOffset + RayGenSize, BaseAlignment);
			uint32_t HitOffset = align_up(MissOffset + MissSize, BaseAlignment);
			uint32_t CallableOffset = align_up(HitOffset + HitSize, BaseAlignment);
			uint32_t TotalSize = CallableOffset + CallableSize;

			// Get Index Map.
			std::vector<uint32_t> RayGenIndices;
			std::vector<uint32_t> MissIndices;
			std::vector<uint32_t> HitIndices;
			std::vector<uint32_t> CallableIndices;
			for (size_t i = 0; i < RSGCI.size(); i++) {  // Iterate through shader groups
				switch (RSGCI[i].type) {
				case VK_RAY_TRACING_SHADER_GROUP_TYPE_GENERAL_KHR:
					if (aRaytracer->ShaderGroup[i].GeneralShader) {
						switch (aRaytracer->ShaderGroup[i].GeneralShader->Stage) {
						case shader::stage::RAYGEN:
							RayGenIndices.push_back(i);  // i is the shader group index
							break;
						case shader::stage::MISS:
							MissIndices.push_back(i);
							break;
						case shader::stage::CALLABLE:
							CallableIndices.push_back(i);
							break;
						default:
							break;
						}
					}
					break;
				case VK_RAY_TRACING_SHADER_GROUP_TYPE_TRIANGLES_HIT_GROUP_KHR:
				case VK_RAY_TRACING_SHADER_GROUP_TYPE_PROCEDURAL_HIT_GROUP_KHR:
					HitIndices.push_back(i);  // i is the shader group index
					break;
				default:
					break;
				}
			}
			
			// Load Host memory object with handles.
			std::vector<uint8_t> SBTHM(TotalSize);
			for (size_t i = 0; i < RayGenIndices.size(); i++) {
				memcpy(
					SBTHM.data() + RayGenOffset + i*HandleSizeAligned,
					ShaderGroupHandle.data() + RayGenIndices[i]*HandleSize, 
					HandleSize
				);
			}
			for (size_t i = 0; i < MissIndices.size(); i++) {
				memcpy(
					SBTHM.data() + MissOffset + i*HandleSizeAligned,
					ShaderGroupHandle.data() + MissIndices[i]*HandleSize, 
					HandleSize
				);
			}
			for (size_t i = 0; i < HitIndices.size(); i++) {
				memcpy(
					SBTHM.data() + HitOffset + i*HandleSizeAligned,
					ShaderGroupHandle.data() + HitIndices[i]*HandleSize, 
					HandleSize
				);
			}
			for (size_t i = 0; i < CallableIndices.size(); i++) {
				memcpy(
					SBTHM.data() + CallableOffset + i*HandleSizeAligned,
					ShaderGroupHandle.data() + CallableIndices[i]*HandleSize, 
					HandleSize
				);
			}
			
			// Create Shader Binding Table.
			buffer::create_info SBTCI{};
			SBTCI.Usage = buffer::usage::SHADER_BINDING_TABLE_KHR | buffer::usage::SHADER_DEVICE_ADDRESS_KHR | buffer::usage::TRANSFER_DST | buffer::usage::TRANSFER_SRC;
			SBTCI.Memory = device::memory::DEVICE_LOCAL;

			// Create Actual Buffer.
			this->ShaderBindingTable.Buffer = aContext->create<buffer>(SBTCI, TotalSize, SBTHM.data());

			// Get addresses for each region for vkCmdTraceRaysKHR().
			VkDeviceAddress SBTBaseAddress = this->ShaderBindingTable.Buffer->device_address();

			this->ShaderBindingTable.Raygen.deviceAddress 		= SBTBaseAddress + RayGenOffset;
			this->ShaderBindingTable.Raygen.stride 				= HandleSizeAligned;
			this->ShaderBindingTable.Raygen.size 				= RayGenSize;

			this->ShaderBindingTable.Miss.deviceAddress 		= SBTBaseAddress + MissOffset;
			this->ShaderBindingTable.Miss.stride 				= HandleSizeAligned;
			this->ShaderBindingTable.Miss.size 					= MissSize;

			this->ShaderBindingTable.Hit.deviceAddress 			= SBTBaseAddress + HitOffset;
			this->ShaderBindingTable.Hit.stride 				= HandleSizeAligned;
			this->ShaderBindingTable.Hit.size 					= HitSize;

			this->ShaderBindingTable.Callable.deviceAddress 	= SBTBaseAddress + CallableOffset;
			this->ShaderBindingTable.Callable.stride 			= HandleSizeAligned;
			this->ShaderBindingTable.Callable.size 				= CallableSize;
		}
	}

	pipeline::pipeline(std::shared_ptr<context> aContext, std::shared_ptr<compute> aCompute) : pipeline() {
		VkResult Result = VK_SUCCESS;
		PFN_vkCreateComputePipelines vkCreateComputePipelines = (PFN_vkCreateComputePipelines)aContext->function_pointer("vkCreateComputePipelines");

		this->CreateInfo	= aCompute;
		this->Context		= aContext;
		this->BindPoint		= VK_PIPELINE_BIND_POINT_COMPUTE;

		// Create Respective Shader Modules.
		Result = this->shader_stage_create(aCompute);

		// Generate Descriptor Set Layouts from Meta Data gathered from shaders, create pipeline layout.
		Result = this->create_pipeline_layout(aCompute->DescriptorSetLayoutBinding);

		VkComputePipelineCreateInfo CPCI{};
		CPCI.sType						= VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
		CPCI.pNext						= NULL;
		CPCI.flags						= 0;
		CPCI.stage						= this->Stage[0];
		CPCI.layout						= this->Layout;
		CPCI.basePipelineHandle			= VK_NULL_HANDLE;
		CPCI.basePipelineIndex			= 0;

		Result = vkCreateComputePipelines(aContext->Handle, this->Cache, 1, &CPCI, NULL, &this->Handle);
	}

	pipeline::~pipeline() {
		PFN_vkDestroyPipeline vkDestroyPipeline = (PFN_vkDestroyPipeline)this->Context->function_pointer("vkDestroyPipeline");
		PFN_vkDestroyPipelineCache vkDestroyPipelineCache = (PFN_vkDestroyPipelineCache)this->Context->function_pointer("vkDestroyPipelineCache");
		PFN_vkDestroyPipelineLayout vkDestroyPipelineLayout = (PFN_vkDestroyPipelineLayout)this->Context->function_pointer("vkDestroyPipelineLayout");
		PFN_vkDestroyDescriptorPool vkDestroyDescriptorPool = (PFN_vkDestroyDescriptorPool)this->Context->function_pointer("vkDestroyDescriptorPool");
		PFN_vkDestroyDescriptorSetLayout vkDestroyDescriptorSetLayout = (PFN_vkDestroyDescriptorSetLayout)this->Context->function_pointer("vkDestroyDescriptorSetLayout");
		PFN_vkDestroyShaderModule vkDestroyShaderModule = (PFN_vkDestroyShaderModule)this->Context->function_pointer("vkDestroyShaderModule");
		PFN_vkDestroyRenderPass vkDestroyRenderPass = (PFN_vkDestroyRenderPass)this->Context->function_pointer("vkDestroyRenderPass");

		// Delete all vulkan allocated resources.
		if (this->Handle != VK_NULL_HANDLE) {
			vkDestroyPipeline(this->Context->Handle, this->Handle, NULL);
		}
		if (this->Cache != VK_NULL_HANDLE) {
			vkDestroyPipelineCache(this->Context->Handle, this->Cache, NULL);
		}
		if (this->Layout != VK_NULL_HANDLE) {
			vkDestroyPipelineLayout(this->Context->Handle, this->Layout, NULL);
		}
		if (this->DescriptorPool != VK_NULL_HANDLE) {
			vkDestroyDescriptorPool(this->Context->Handle, this->DescriptorPool, NULL);
		}
		if (this->DescriptorSetLayout.size() != 0) {
			for (size_t i = 0; i < this->DescriptorSetLayout.size(); i++) {
				vkDestroyDescriptorSetLayout(this->Context->Handle, this->DescriptorSetLayout[i], NULL);
			}
		}
		if (this->Stage.size() != 0) {
			for (size_t i = 0; i < this->Stage.size(); i++) {
				vkDestroyShaderModule(this->Context->Handle, this->Stage[i].module, NULL);
			}
		}
		if (this->RenderPass != VK_NULL_HANDLE) {
			vkDestroyRenderPass(this->Context->Handle, this->RenderPass, NULL);
		}		
	}

	void pipeline::bind(
		command_buffer* 						aCommandBuffer, 
		std::vector<std::shared_ptr<buffer>> 	aVertexBuffer, 
		std::shared_ptr<buffer> 				aIndexBuffer, 
		std::shared_ptr<descriptor::array> 		aDescriptorArray
	) {
		// Load function pointers onto stack
		PFN_vkCmdBindPipeline vkCmdBindPipeline = (PFN_vkCmdBindPipeline)this->Context->function_pointer("vkCmdBindPipeline");
		PFN_vkCmdBindDescriptorSets vkCmdBindDescriptorSets = (PFN_vkCmdBindDescriptorSets)this->Context->function_pointer("vkCmdBindDescriptorSets");
		PFN_vkCmdBindVertexBuffers vkCmdBindVertexBuffers = (PFN_vkCmdBindVertexBuffers)this->Context->function_pointer("vkCmdBindVertexBuffers");
		PFN_vkCmdBindIndexBuffer vkCmdBindIndexBuffer = (PFN_vkCmdBindIndexBuffer)this->Context->function_pointer("vkCmdBindIndexBuffer");
		// Bind resources to pipeline.
		vkCmdBindPipeline(aCommandBuffer->Handle, this->BindPoint, this->Handle);
		if ((aDescriptorArray != nullptr) ? (aDescriptorArray->DescriptorSet.size() > 0) : false) {
			vkCmdBindDescriptorSets(aCommandBuffer->Handle, this->BindPoint, this->Layout, 0, aDescriptorArray->DescriptorSet.size(), aDescriptorArray->DescriptorSet.data(), 0, NULL);
		}
		if (aVertexBuffer.size() > 0) {
			std::vector<VkBuffer> VertexBuffer(aVertexBuffer.size());
			std::vector<VkDeviceSize> VertexBufferOffset(aVertexBuffer.size(), 0);
			for (size_t i = 0; i < aVertexBuffer.size(); i++) {
				VertexBuffer[i] = aVertexBuffer[i]->Handle;
			}
			vkCmdBindVertexBuffers(aCommandBuffer->Handle, 0, VertexBuffer.size(), VertexBuffer.data(), VertexBufferOffset.data());
		}
		if (aIndexBuffer != nullptr) {
			VkIndexType IndexType = (aVertexBuffer[0]->ElementCount <= (1 << 16)) ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;
			vkCmdBindIndexBuffer(aCommandBuffer->Handle, aIndexBuffer->Handle, 0, IndexType);
		}
	}

	void pipeline::begin(
		command_buffer* 				aCommandBuffer, 
		std::shared_ptr<framebuffer> 	aFramebuffer, 
		VkRect2D 						aRenderArea, 
		VkSubpassContents 				aSubpassContents
	) {
		PFN_vkCmdBeginRenderPass vkCmdBeginRenderPass = (PFN_vkCmdBeginRenderPass)this->Context->function_pointer("vkCmdBeginRenderPass");
		VkRenderPassBeginInfo RPBI{};
		RPBI.sType				= VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		RPBI.pNext				= NULL;
		RPBI.renderPass			= this->RenderPass;
		RPBI.framebuffer		= aFramebuffer->Handle;
		RPBI.renderArea			= aRenderArea;
		RPBI.clearValueCount	= aFramebuffer->ClearValue.size();
		RPBI.pClearValues		= aFramebuffer->ClearValue.data();
		vkCmdBeginRenderPass(aCommandBuffer->Handle, &RPBI, aSubpassContents);
	}

	void pipeline::rasterize(
		command_buffer* 											aCommandBuffer,
		std::shared_ptr<framebuffer> 								aFramebuffer,
		std::array<unsigned int, 3> 								aResolution,
		std::vector<std::shared_ptr<buffer>> 						aVertexBuffer,
		std::shared_ptr<buffer> 									aIndexBuffer,
		std::shared_ptr<descriptor::array> 							aDescriptorArray
	) {
		// So if I designate a render area, the render pass will only affect those fragments for each attachment in the framebuffer.
		// Say a pipeline in a subpass describes it will only render to a subset in the frame buffer, it will then map NDC to the 
		// described sections of pixels, but anything outside of the render area will be ignored. Scissor operations act as a union 
		// on a pipeline level, but with the render area, what ever region of pixels the scissors choose, it must ultimately be inside 
		// the render area?
		PFN_vkCmdDrawIndexed vkCmdDrawIndexed = (PFN_vkCmdDrawIndexed)this->Context->function_pointer("vkCmdDrawIndexed");
		PFN_vkCmdSetViewport vkCmdSetViewport = (PFN_vkCmdSetViewport)this->Context->function_pointer("vkCmdSetViewport");
		PFN_vkCmdSetScissor vkCmdSetScissor = (PFN_vkCmdSetScissor)this->Context->function_pointer("vkCmdSetScissor");
		PFN_vkCmdDraw vkCmdDraw = (PFN_vkCmdDraw)this->Context->function_pointer("vkCmdDraw");
		// TODO: This can be expanded for multiple viewports and scissors later.
		// Determines the render area for the render pass.
		VkRect2D RenderArea 	= { { 0, 0 }, { aResolution[0], aResolution[1] } };
		// Determines the viewport output for the pipeline.
		VkViewport Viewport 	= { 0.0f, 0.0f, (float)aResolution[0], (float)aResolution[1], 0.0f, 1.0f };
		// Much like render area, determines where fragments will be output to at pipeline level.
		VkRect2D Scissor 		= { {0, 0}, {aResolution[0], aResolution[1]} };
		this->begin(aCommandBuffer, aFramebuffer, RenderArea);
		this->bind(aCommandBuffer, aVertexBuffer, aIndexBuffer, aDescriptorArray);		
		vkCmdSetViewport(aCommandBuffer->Handle, 0, 1, &Viewport);
		vkCmdSetScissor(aCommandBuffer->Handle, 0, 1, &Scissor);
		if (aIndexBuffer != nullptr) {
			vkCmdDrawIndexed(aCommandBuffer->Handle, aIndexBuffer->ElementCount, 1, 0, 0, 0);
		}
		else {
			vkCmdDraw(aCommandBuffer->Handle, aVertexBuffer[0]->ElementCount, 1, 0, 0);
		}
		this->end(aCommandBuffer);
	}

	void pipeline::end(command_buffer* aCommandBuffer) {
		PFN_vkCmdEndRenderPass vkCmdEndRenderPass = (PFN_vkCmdEndRenderPass)this->Context->function_pointer("vkCmdEndRenderPass");
		vkCmdEndRenderPass(aCommandBuffer->Handle);
	}

	void pipeline::raytrace(
		command_buffer* 											aCommandBuffer,
		std::array<unsigned int, 3> 								aResolution,
		std::shared_ptr<descriptor::array> 							aDescriptorArray
	) {
		PFN_vkCmdTraceRaysKHR vkCmdTraceRaysKHR = (PFN_vkCmdTraceRaysKHR)this->Context->function_pointer("vkCmdTraceRaysKHR");
		this->bind(aCommandBuffer, {}, nullptr, aDescriptorArray);
		vkCmdTraceRaysKHR(
			aCommandBuffer->Handle, 
			&this->ShaderBindingTable.Raygen,
			&this->ShaderBindingTable.Miss,
			&this->ShaderBindingTable.Hit,
			&this->ShaderBindingTable.Callable,
			aResolution[0], 
			aResolution[1], 
			aResolution[2]
		);
	}

	void pipeline::dispatch(
		command_buffer* 				 							aCommandBuffer,
		std::array<unsigned int, 3> 								aThreadGroupCount,
		std::shared_ptr<descriptor::array> 							aDescriptorArray
	) {
		PFN_vkCmdDispatch vkCmdDispatch = (PFN_vkCmdDispatch)this->Context->function_pointer("vkCmdDispatch");
		this->bind(aCommandBuffer, {}, nullptr, aDescriptorArray);
		vkCmdDispatch(aCommandBuffer->Handle, aThreadGroupCount[0], aThreadGroupCount[1], aThreadGroupCount[2]);
	}

	VkResult pipeline::rasterize(
		std::shared_ptr<framebuffer> 								aFramebuffer,
		std::array<unsigned int, 3> 								aResolution,
		std::vector<std::shared_ptr<buffer>> 						aVertexBuffer,
		std::shared_ptr<buffer> 									aIndexBuffer,
		std::shared_ptr<descriptor::array> 							aDescriptorArray
	) {
		VkResult Result = VK_SUCCESS;

		auto CommandPool = this->Context->create<command_pool>(device::operation::GRAPHICS);
		auto CommandBuffer = CommandPool->create<command_buffer>();

		Result = CommandBuffer->begin();
		this->rasterize(CommandBuffer.get(), aFramebuffer, aResolution, aVertexBuffer, aIndexBuffer, aDescriptorArray);
		Result = CommandBuffer->end();

		Result = this->Context->execute_and_wait(device::operation::GRAPHICS, CommandBuffer);

		return Result;
	}

	VkResult pipeline::rasterize(
		std::vector<std::shared_ptr<image>> 						aImage,
		std::array<unsigned int, 3> 								aResolution,
		std::vector<std::shared_ptr<buffer>> 						aVertexBuffer,
		std::shared_ptr<buffer> 									aIndexBuffer,
		std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding
	) {
		// Error code tracking.
		VkResult Result = VK_SUCCESS;

		// TODO: Get Rasterizer resolution, change later.
		std::shared_ptr<rasterizer> Rasterizer = std::dynamic_pointer_cast<rasterizer>(this->CreateInfo);

		// Allocated GPU Resources needed to execute.
		auto CommandPool = this->Context->create<command_pool>(device::operation::GRAPHICS);
		auto CommandBuffer = CommandPool->create<command_buffer>();
		auto Framebuffer = this->Context->create<framebuffer>(this->shared_from_this(), aImage, aResolution);
		auto DescriptorArray = this->Context->create<descriptor::array>(this->shared_from_this());

		// Bind Resources to Descriptor Sets
		for (auto& [SetBinding, Resource] : aUniformSetBinding) {
			switch(Resource->Type) {
			case resource::type::BUFFER: {
				// Bind Buffer resources
				std::shared_ptr<buffer> BufferResource = std::dynamic_pointer_cast<buffer>(Resource);
				DescriptorArray->bind(SetBinding.first, SetBinding.second, 0, BufferResource->Handle);
				}
				break;
			case resource::type::IMAGE: {
				// Bind Image resources
				std::shared_ptr<image> ImageResource = std::dynamic_pointer_cast<image>(Resource);
				DescriptorArray->bind(SetBinding.first, SetBinding.second, 0, ImageResource->View);
				}
				break;
			default:
				// Unsupported resource type
				break;
			}
		}

		// Write Command Buffer here.
		Result = CommandBuffer->begin();
		this->rasterize(CommandBuffer.get(), Framebuffer, aResolution, aVertexBuffer, aIndexBuffer, DescriptorArray);
		Result = CommandBuffer->end();

		// Execute Command Buffer here.
		Result = this->Context->execute_and_wait(device::operation::GRAPHICS, CommandBuffer);

		return Result;
	}

	VkResult pipeline::raytrace(
		std::shared_ptr<image> 										aOutputImage,
		std::shared_ptr<acceleration_structure> 					aTLAS,
		std::array<unsigned int, 3> 								aResolution,
		std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding
	) {
		VkResult Result = VK_SUCCESS;
		
		auto CommandPool = this->Context->create<command_pool>(device::operation::GRAPHICS);
		auto CommandBuffer = CommandPool->create<command_buffer>();
		auto DescriptorArray = this->Context->create<descriptor::array>(this->shared_from_this());

		// Bind Resources to Descriptor Sets
		for (auto& [SetBinding, Resource] : aUniformSetBinding) {
			switch(Resource->Type) {
			case resource::type::BUFFER: {
				// Bind Buffer resources
				std::shared_ptr<buffer> BufferResource = std::dynamic_pointer_cast<buffer>(Resource);
				DescriptorArray->bind(SetBinding.first, SetBinding.second, 0, BufferResource->Handle);
				}
				break;
			case resource::type::IMAGE: {
				// Bind Image resources
				std::shared_ptr<image> ImageResource = std::dynamic_pointer_cast<image>(Resource);
				DescriptorArray->bind(SetBinding.first, SetBinding.second, 0, ImageResource->View);
				}
				break;
			default:
				// Unsupported resource type
				break;
			}
		}

		Result = CommandBuffer->begin();
		this->raytrace(CommandBuffer.get(), aResolution, DescriptorArray);
		Result = CommandBuffer->end();

		// Execute Command Buffer here.
		Result = this->Context->execute_and_wait(device::operation::GRAPHICS, CommandBuffer);

		return Result;
	}

	VkResult pipeline::dispatch(
		std::array<unsigned int, 3> 								aThreadGroupCount,
		std::shared_ptr<descriptor::array> 							aDescriptorArray
	) {
		VkResult Result = VK_SUCCESS;
		
		auto CommandPool = this->Context->create<command_pool>(device::operation::COMPUTE);
		auto CommandBuffer = CommandPool->create<command_buffer>();

		Result = CommandBuffer->begin();
		this->raytrace(CommandBuffer.get(), aThreadGroupCount, aDescriptorArray);
		Result = CommandBuffer->end();

		// Execute Command Buffer here.
		Result = this->Context->execute_and_wait(device::operation::COMPUTE, CommandBuffer);

		return Result;
	}

	VkResult pipeline::dispatch(
		std::array<unsigned int, 3> 								aThreadGroupCount,
		std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding
	) {
		VkResult Result = VK_SUCCESS;
		
		auto CommandPool = this->Context->create<command_pool>(device::operation::COMPUTE);
		auto CommandBuffer = CommandPool->create<command_buffer>();
		auto DescriptorArray = this->Context->create<descriptor::array>(this->shared_from_this());

		// Bind Resources to Descriptor Sets
		for (auto& [SetBinding, Resource] : aUniformSetBinding) {
			switch(Resource->Type) {
			case resource::type::BUFFER: {
				// Bind Buffer resources
				std::shared_ptr<buffer> BufferResource = std::dynamic_pointer_cast<buffer>(Resource);
				DescriptorArray->bind(SetBinding.first, SetBinding.second, 0, BufferResource->Handle);
				}
				break;
			case resource::type::IMAGE: {
				// Bind Image resources
				std::shared_ptr<image> ImageResource = std::dynamic_pointer_cast<image>(Resource);
				DescriptorArray->bind(SetBinding.first, SetBinding.second, 0, ImageResource->View);
				}
				break;
			default:
				// Unsupported resource type
				break;
			}
		}

		Result = CommandBuffer->begin();
		this->raytrace(CommandBuffer.get(), aThreadGroupCount, DescriptorArray);
		Result = CommandBuffer->end();

		// Execute Command Buffer here.
		Result = this->Context->execute_and_wait(device::operation::COMPUTE, CommandBuffer);

		return Result;
	}

	std::vector<VkDescriptorPoolSize> pipeline::descriptor_pool_sizes() const {
		std::map<VkDescriptorType, uint32_t> DescriptorTypeCount = this->descriptor_type_count();
		// Convert to pool size to vector data structure.
		std::vector<VkDescriptorPoolSize> PoolSize;
		for (auto& [Type, Count] : DescriptorTypeCount) {
			VkDescriptorPoolSize DPS{};
			DPS.type = Type;
			DPS.descriptorCount = Count;
			PoolSize.push_back(DPS);
		}

		return PoolSize;
	}

	std::map<VkDescriptorType, uint32_t> pipeline::descriptor_type_count() const {
		// Calculate pool sizes based on pipeline descriptor set layout binding info.
		std::map<VkDescriptorType, uint32_t> DescriptorTypeCount;
		for (size_t j = 0; j < this->CreateInfo->DescriptorSetLayoutBinding.size(); j++) {
			for (size_t k = 0; k < this->CreateInfo->DescriptorSetLayoutBinding[j].size(); k++) {
				VkDescriptorSetLayoutBinding DSLB = this->CreateInfo->DescriptorSetLayoutBinding[j][k];
				if (DescriptorTypeCount.count(DSLB.descriptorType) == 0) {
					DescriptorTypeCount[DSLB.descriptorType] = 0;
				}
				DescriptorTypeCount[DSLB.descriptorType] += DSLB.descriptorCount;
			}
		}
		return DescriptorTypeCount;
	}

	std::vector<std::vector<VkDescriptorSetLayoutBinding>> pipeline::descriptor_set_layout_binding() const {
		return this->CreateInfo->DescriptorSetLayoutBinding;
	}

	VkResult pipeline::shader_stage_create(std::shared_ptr<create_info> aCreateInfo) {
		VkResult Result = VK_SUCCESS;
		PFN_vkCreateShaderModule vkCreateShaderModule = (PFN_vkCreateShaderModule)this->Context->function_pointer("vkCreateShaderModule");
		// Generate GPU Shader Modules.
		this->Stage = std::vector<VkPipelineShaderStageCreateInfo>(aCreateInfo->Shader.size());
		for (size_t i = 0; i < this->Stage.size(); i++) {
			// Generate Shader Module Info
			VkShaderModuleCreateInfo ShaderModuleCreateInfo{};
			ShaderModuleCreateInfo.sType				= VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
			ShaderModuleCreateInfo.pNext				= NULL;
			ShaderModuleCreateInfo.flags				= 0;
			ShaderModuleCreateInfo.codeSize				= aCreateInfo->ByteCode[i].size() * sizeof(unsigned int);
			ShaderModuleCreateInfo.pCode				= aCreateInfo->ByteCode[i].data();
			// Load Shader Stage Meta Data
			this->Stage[i] 								= aCreateInfo->Shader[i]->pipeline_shader_stage_create_info();
			// Create Shader Module
			Result = vkCreateShaderModule(Context->Handle, &ShaderModuleCreateInfo, NULL, &this->Stage[i].module);
		}
		return Result;
	}

	VkResult pipeline::create_pipeline_layout(std::vector<std::vector<VkDescriptorSetLayoutBinding>> aDescriptorSetLayoutBinding) {
		VkResult Result = VK_SUCCESS;
		PFN_vkCreateDescriptorSetLayout vkCreateDescriptorSetLayout = (PFN_vkCreateDescriptorSetLayout)this->Context->function_pointer("vkCreateDescriptorSetLayout");
		PFN_vkCreateDescriptorPool vkCreateDescriptorPool = (PFN_vkCreateDescriptorPool)this->Context->function_pointer("vkCreateDescriptorPool");
		PFN_vkCreatePipelineLayout vkCreatePipelineLayout = (PFN_vkCreatePipelineLayout)this->Context->function_pointer("vkCreatePipelineLayout");

		// Generate Descriptor Set Layouts from Meta Data gathered from shaders.
		if (Result == VK_SUCCESS) {
			this->DescriptorSetLayout = std::vector<VkDescriptorSetLayout>(aDescriptorSetLayoutBinding.size());
			for (size_t i = 0; i < aDescriptorSetLayoutBinding.size(); i++) {
				VkDescriptorSetLayoutCreateInfo CreateInfo{};
				CreateInfo.sType			= VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
				CreateInfo.pNext			= NULL;
				CreateInfo.flags			= 0;
				if (aDescriptorSetLayoutBinding[i].size() > 0) {
					CreateInfo.bindingCount		= aDescriptorSetLayoutBinding[i].size();
					CreateInfo.pBindings		= aDescriptorSetLayoutBinding[i].data();
				}
				else {
					CreateInfo.bindingCount		= 0;
					CreateInfo.pBindings		= NULL;
				}
				// ! NOTE: We got our answer, we cannot use VK_NULL_HANDLE for empty descriptor set layouts. We have to 
				// ! construct them even if they are empty. The only exception is if extension VK_EXT_graphics_pipeline_library 
				// ! is enabled, then we can use VK_NULL_HANDLE to construct pipeline layouts and descriptor sets.
				Result = vkCreateDescriptorSetLayout(Context->Handle, &CreateInfo, NULL, &this->DescriptorSetLayout[i]);
			}
		}

		// Generate Descriptor Set Pool.
		if ((Result == VK_SUCCESS) && (aDescriptorSetLayoutBinding.size() > 0)) {
			std::vector<VkDescriptorPoolSize> DescriptorPoolSize = this->descriptor_pool_sizes();
			VkDescriptorPoolCreateInfo DPCI{};
			DPCI.sType 				= VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
			DPCI.pNext 				= NULL;
			DPCI.flags 				= VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			DPCI.maxSets 			= aDescriptorSetLayoutBinding.size();
			DPCI.poolSizeCount 		= DescriptorPoolSize.size();
			DPCI.pPoolSizes 		= DescriptorPoolSize.data();
			Result = vkCreateDescriptorPool(Context->Handle, &DPCI, NULL, &this->DescriptorPool);
		}

		// Create Pipeline Layout.
		if (Result == VK_SUCCESS) {
			// Create Pipeline Layout.
			VkPipelineLayoutCreateInfo CreateInfo{};
			CreateInfo.sType						= VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
			CreateInfo.pNext						= NULL;
			CreateInfo.flags						= 0;
			CreateInfo.setLayoutCount				= this->DescriptorSetLayout.size();
			CreateInfo.pSetLayouts					= this->DescriptorSetLayout.data();
			CreateInfo.pushConstantRangeCount		= 0;
			CreateInfo.pPushConstantRanges			= NULL;

			Result = vkCreatePipelineLayout(this->Context->Handle, &CreateInfo, NULL, &this->Layout);
		}

		return Result;
	}

}
