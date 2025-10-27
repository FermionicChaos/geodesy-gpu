#ifndef GEODESY_GPU_EXECUTABLE_CALL_H
#define GEODESY_GPU_EXECUTABLE_CALL_H

#include "config.h"

#include "device.h"
#include "resource.h"
#include "fence.h"
#include "semaphore.h"
#include "semaphore_pool.h"
#include "command_buffer.h"
#include "command_pool.h"
#include "command_batch.h"
#include "buffer.h"
#include "image.h"
#include "shader.h"
#include "descriptor.h"
#include "framebuffer.h"
#include "acceleration_structure.h"
#include "pipeline.h"
#include "framechain.h"
#include "swapchain.h"

namespace geodesy::gpu {

	// This is a container class for a singular executable call to a gpu queue.
	// This could be a rasterization draw call, a compute dispatch call, or a
	// ray tracing call. This class will contain all the necessary metadata
	// to facilitate a full call.
	class executable_call : public command_buffer {
	public:

		std::shared_ptr<pipeline> 					Pipeline;
		std::shared_ptr<framebuffer> 				Framebuffer;
		std::shared_ptr<descriptor::array> 			DescriptorArray;

		executable_call(std::shared_ptr<context> aContext, std::shared_ptr<command_pool> aCommandPool);

	};

	class rasterization_call : public executable_call {
	public:

		rasterization_call(
			std::shared_ptr<context> 									aContext,
			std::shared_ptr<command_pool> 								aCommandPool,
			std::shared_ptr<pipeline> 									aRasterizationPipeline,
			std::vector<std::shared_ptr<image>> 						aImage,
			std::array<unsigned int, 3> 								aResolution,
			std::vector<std::shared_ptr<buffer>> 						aVertexBuffer = {},
			std::shared_ptr<buffer> 									aIndexBuffer = nullptr,
			std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding = {}
		);

	};

	class raytracing_call : public executable_call {
	public:

		raytracing_call(
			std::shared_ptr<context> 									aContext,
			std::shared_ptr<command_pool> 								aCommandPool,
			std::shared_ptr<pipeline> 									aRaytracerPipeline,
			std::array<unsigned int, 3> 								aResolution,
			std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding = {}
		);

	};

	class compute_call : public executable_call {
	public:

		compute_call(
			std::shared_ptr<context> 									aContext,
			std::shared_ptr<command_pool> 								aCommandPool,
			std::shared_ptr<pipeline> 									aComputePipeline,
			std::array<unsigned int, 3> 								aThreadGroupCount,
			std::map<std::pair<int, int>, std::shared_ptr<resource>> 	aUniformSetBinding = {}
		);
		
	};

}

#endif // !GEODESY_GPU_EXECUTABLE_CALL_H