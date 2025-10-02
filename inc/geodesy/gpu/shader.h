#pragma once
#ifndef GEODESY_GPU_SHADER_H
#define GEODESY_GPU_SHADER_H

#include "config.h"

namespace geodesy::gpu {

	class shader /*: public io::file, public glslang::TIntermTraverser*/ {
	public:

		friend class pipeline;

		enum stage {
			UNKNOWN,
    		VERTEX								= VK_SHADER_STAGE_VERTEX_BIT,
    		TESSELLATION_CONTROL				= VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
    		TESSELLATION_EVALUATION				= VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
    		GEOMETRY							= VK_SHADER_STAGE_GEOMETRY_BIT,
    		FRAGMENT							= VK_SHADER_STAGE_FRAGMENT_BIT,
    		COMPUTE								= VK_SHADER_STAGE_COMPUTE_BIT,
    		RAYGEN							 	= VK_SHADER_STAGE_RAYGEN_BIT_KHR,
    		ANY_HIT							 	= VK_SHADER_STAGE_ANY_HIT_BIT_KHR,
    		CLOSEST_HIT						 	= VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR,
    		MISS							 	= VK_SHADER_STAGE_MISS_BIT_KHR,
    		INTERSECTION					 	= VK_SHADER_STAGE_INTERSECTION_BIT_KHR,
    		CALLABLE						 	= VK_SHADER_STAGE_CALLABLE_BIT_KHR,
			PIXEL								= FRAGMENT
		};

		// ---------- Host Memory Object ---------- //
		stage								Stage;
		std::shared_ptr<glslang::TShader> 	Handle;

		shader();
		// shader(std::string aFilePath);
		shader(stage aShaderStage, std::string aSourceCode);
		
		VkShaderStageFlagBits get_stage();
		VkPipelineShaderStageCreateInfo pipeline_shader_stage_create_info();

	private:

		static bool initialize();
		static void terminate();

		// TODO: Rename to something else, does not generate SPIRV.
		bool compile_source(stage aShaderStage, std::string aSourceCode);

	};

}

#endif // !GEODESY_GPU_SHADER_H
