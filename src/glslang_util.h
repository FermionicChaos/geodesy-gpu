#pragma once
#ifndef GEODESY_GPU_GLSLANG_UTIL_H
#define GEODESY_GPU_GLSLANG_UTIL_H

#include <assert.h>

#include <geodesy/gpu/config.h>

//#include <glslang/glslang/Include/arrays.h>
#include <glslang/glslang/Include/BaseTypes.h>
#include <glslang/glslang/Include/Common.h>
#include <glslang/glslang/Include/ConstantUnion.h>
#include <glslang/glslang/Include/intermediate.h>
#include <glslang/glslang/Include/PoolAlloc.h>
#include <glslang/glslang/Include/ResourceLimits.h>
#include <glslang/glslang/Include/SpirvIntrinsics.h>
#include <glslang/glslang/Include/Types.h>

#include <glslang/glslang/MachineIndependent/localintermediate.h>

#include <glslang/glslang/Public/ShaderLang.h>

// Converts shader source into SPIRV.
//#include <glslang/SPIRV/SPIRV/SpvTools.h>
//#include <glslang/SPIRV/SPIRV/Logger.h>
#include <glslang/SPIRV/GlslangToSpv.h>
//#include <glslang/SPIRV/SPIRV/spirv.hpp>
//#include <glslang/SPIRV/SPIRV/spvIR.h>
//#include <glslang/SPIRV/SPIRV/SPVRemapper.h>

// Included for compiling
#include "ResourceLimits.h"

namespace geodesy::gpu {

	EShLanguage vulkan_to_glslang(VkShaderStageFlagBits aStage);
	VkShaderStageFlagBits vulkan_to_glslang(EShLanguage aStage);

	VkShaderStageFlags glslang_shader_stage_to_vulkan(EShLanguageMask aShaderStage);
	// util::variable convert_to_variable(const glslang::TType* aType, const char* aName);

}

#endif // !GEODESY_GPU_GLSLANG_UTIL_H
