#pragma once
#ifndef GEODESY_CORE_GPU_ACCELERATION_STRUCTURE_H
#define GEODESY_CORE_GPU_ACCELERATION_STRUCTURE_H
/*
An acceleration structure in vulkan is an object that maps out a geometry and optimizes it for ray tracing.
This is a bottom level acceleration structure that maps out the geometry of a mesh.
*/

#include "config.h"

#include "resource.h"
#include "buffer.h"

namespace geodesy::gpu {

	class acceleration_structure : public resource {
	public:

		std::shared_ptr<buffer> Buffer;
		std::shared_ptr<buffer> UpdateScratchBuffer;
		std::shared_ptr<buffer> BuildScratchBuffer;
		std::shared_ptr<buffer> InstanceBuffer;
		VkAccelerationStructureKHR Handle;
		VkDeviceAddress DeviceAddress;

		acceleration_structure();
		acceleration_structure(
			std::shared_ptr<context> aContext,
			std::shared_ptr<buffer> aVertexBuffer,
			size_t aVertexPositionOffset,
			VkFormat aVertexPositionFormat,
			size_t aVertexStride,
			size_t aVertexCount,
			std::shared_ptr<buffer> aIndexBuffer,
			VkIndexType aIndexType,
			size_t aTriangleCount
		);
		acceleration_structure(
			std::shared_ptr<context> aContext,
			const std::vector<VkAccelerationStructureInstanceKHR>& aInstanceList
		);
		~acceleration_structure();

		VkDeviceAddress device_address() const;

	};

}

#endif // GEODESY_CORE_GPU_ACCELERATION_STRUCTURE_H