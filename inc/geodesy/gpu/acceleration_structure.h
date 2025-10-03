#pragma once
#ifndef GEODESY_CORE_GPU_ACCELERATION_STRUCTURE_H
#define GEODESY_CORE_GPU_ACCELERATION_STRUCTURE_H
/*
An acceleration structure in vulkan is an object that maps out a geometry and optimizes it for ray tracing.
This is a bottom level acceleration structure that maps out the geometry of a mesh.
*/

#include "config.h"
#include "buffer.h"

namespace geodesy::gpu {
	
	class acceleration_structure {
	public:

		std::shared_ptr<context> 			Context;
		std::shared_ptr<buffer>				Buffer;
		std::shared_ptr<buffer> 			UpdateScratchBuffer;
		std::shared_ptr<buffer> 			BuildScratchBuffer;
		std::shared_ptr<buffer> 			InstanceBuffer;
		VkAccelerationStructureKHR			Handle;
		VkDeviceAddress						DeviceAddress;

		acceleration_structure();
		// Build Bottom Level AS (Mesh Geometry Data).
		acceleration_structure(
			std::shared_ptr<context> 									aContext,
			std::shared_ptr<buffer> 									aVertexBuffer,				// Vertex Data Buffer
			size_t 														aVertexPositionOffset, 		// Position Vector Offset in Vertex Struct
			VkFormat 													aVertexPositionFormat,		// Position Vector Format
			size_t 														aVertexStride, 				// Size of Vertex Struct
			size_t 														aVertexCount, 				// Number of Vertices in Buffer
			std::shared_ptr<buffer> 									aIndexBuffer, 				// Index Data Buffer
			VkIndexType 												aIndexType,					// Internal Data Type of Indices
			size_t 														aTriangleCount 				// Number of Faces (Triangles)
		);
		// Build Top Level AS (Mesh Instances).
		acceleration_structure(
			std::shared_ptr<context> 									aContext, 
			const std::vector<VkAccelerationStructureInstanceKHR>& 		aInstanceList	// List of Mesh Instances in Scene
		);
		// Clear out resources
		~acceleration_structure();

		VkDeviceAddress device_address() const;

	};

}

#endif // GEODESY_CORE_GPU_ACCELERATION_STRUCTURE_H