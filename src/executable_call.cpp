#include <geodesy/gpu/executable_call.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	executable_call::executable_call() {}

	executable_call::executable_call(
		std::shared_ptr<context> 									aContext,
		std::shared_ptr<command_pool> 								aCommandPool,
		std::shared_ptr<pipeline> 									aRasterizationPipeline,
		std::vector<std::shared_ptr<image>> 						aImage,
		std::vector<std::shared_ptr<buffer>> 						aVertexBuffer,
		std::shared_ptr<buffer> 									aIndexBuffer,
		std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding
	) : command_buffer(aContext, aCommandPool) {
		VkResult Result = VK_SUCCESS;
		std::shared_ptr<pipeline::rasterizer> Rasterizer = std::dynamic_pointer_cast<pipeline::rasterizer>(aRasterizationPipeline->CreateInfo);
		// These objects need to persist longer than the call, so they are passed in.
		this->Pipeline = aRasterizationPipeline;

		// Allocate Framebuffer object metadata.
		this->Framebuffer = aContext->create<framebuffer>(aRasterizationPipeline, aImage, Rasterizer->Resolution);

		// Allocate Descriptor Set Array
		this->DescriptorArray = aContext->create<descriptor::array>(aRasterizationPipeline);

		// Bind Resources to Descriptor Sets
		for (auto& [SetBinding, Resource] : aUniformSetBinding) {
			switch(Resource->Type) {
			case resource::type::BUFFER: {
				// Bind Buffer resources
				std::shared_ptr<buffer> BufferResource = std::dynamic_pointer_cast<buffer>(Resource);
				this->DescriptorArray->bind(SetBinding.first, SetBinding.second, 0, BufferResource->Handle);
				}
				break;
			case resource::type::IMAGE: {
				// Bind Image resources
				std::shared_ptr<image> ImageResource = std::dynamic_pointer_cast<image>(Resource);
				this->DescriptorArray->bind(SetBinding.first, SetBinding.second, 0, ImageResource->View);
				}
				break;
			case resource::type::ACCELERATION_STRUCTURE: {
				// Bind Acceleration Structure resources
				std::shared_ptr<acceleration_structure> AccelerationStructureResource = std::dynamic_pointer_cast<acceleration_structure>(Resource);
				this->DescriptorArray->bind(SetBinding.first, SetBinding.second, 0, AccelerationStructureResource->Handle);
				}
				break;
			}
		}

		// Record to Command Buffer
		Result = this->begin();
		this->Pipeline->rasterize(this, this->Framebuffer, aVertexBuffer, aIndexBuffer, this->DescriptorArray);
		Result = this->end();
	}

}
