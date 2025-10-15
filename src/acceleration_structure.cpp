#include <geodesy/gpu/acceleration_structure.h>

#include <geodesy/gpu/context.h>

namespace geodesy::gpu {

	acceleration_structure::acceleration_structure() {
		// Zero init here.
		this->Context = nullptr;
		this->Type = resource::type::ACCELERATION_STRUCTURE;
		
	}

	acceleration_structure::acceleration_structure(
		std::shared_ptr<context> 				aContext,
		std::shared_ptr<buffer> 				aVertexBuffer,				// Vertex Data Buffer
		size_t 									aVertexPositionOffset, 		// Position Vector Offset in Vertex Struct
		VkFormat 								aVertexPositionFormat,		// Position Vector Format
		size_t 									aVertexStride, 				// Size of Vertex Struct
		size_t 									aVertexCount, 				// Number of Vertices in Buffer
		std::shared_ptr<buffer> 				aIndexBuffer, 				// Index Data Buffer
		VkIndexType 							aIndexType,					// Internal Data Type of Indices
		size_t 									aTriangleCount 				// Number of Faces (Triangles)
	) : acceleration_structure() {
		PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)aContext->function_pointer("vkGetAccelerationStructureBuildSizesKHR");
		PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)aContext->function_pointer("vkCreateAccelerationStructureKHR");
		PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)aContext->function_pointer("vkCmdBuildAccelerationStructuresKHR");
		PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)aContext->function_pointer("vkCmdPipelineBarrier");

		this->Context = aContext;
		uint32_t PrimitiveCount = aTriangleCount;
		// Build Bottom Level AS (Mesh Geometry Data).
		// Describe the geometry of the acceleration structure.
		VkAccelerationStructureGeometryKHR ASG{};
		ASG.sType												= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		ASG.pNext												= NULL;
		ASG.geometryType										= VK_GEOMETRY_TYPE_TRIANGLES_KHR;
		ASG.geometry.triangles.sType							= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_TRIANGLES_DATA_KHR;
		ASG.geometry.triangles.pNext							= NULL;
		ASG.geometry.triangles.vertexFormat						= VK_FORMAT_R32G32B32_SFLOAT;
		ASG.geometry.triangles.vertexData.deviceAddress 		= aVertexBuffer->device_address();
		ASG.geometry.triangles.vertexStride						= aVertexStride;
		ASG.geometry.triangles.maxVertex						= aVertexCount;
		ASG.geometry.triangles.indexType						= aIndexType;
		ASG.geometry.triangles.indexData.deviceAddress 			= aIndexBuffer->device_address();
		ASG.geometry.triangles.transformData.deviceAddress		= 0;
		ASG.flags												= VK_GEOMETRY_OPAQUE_BIT_KHR;

		// Needed for acceleration structure creation.
		VkAccelerationStructureBuildGeometryInfoKHR ASBGI{};
		ASBGI.sType												= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		ASBGI.pNext												= NULL;
		ASBGI.type												= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		ASBGI.flags												= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR;
		ASBGI.mode												= VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		ASBGI.srcAccelerationStructure							= VK_NULL_HANDLE;
		ASBGI.dstAccelerationStructure							= VK_NULL_HANDLE;
		ASBGI.geometryCount										= 1;
		ASBGI.pGeometries										= &ASG;
		ASBGI.ppGeometries										= NULL;
		ASBGI.scratchData.deviceAddress							= 0;

		// Get the required sizes for the acceleration structure.
		VkAccelerationStructureBuildSizesInfoKHR ASBSI{};
		ASBSI.sType												= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		ASBSI.pNext												= NULL;
		vkGetAccelerationStructureBuildSizesKHR(aContext->Handle, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &ASBGI, &PrimitiveCount, &ASBSI);

		// Build the acceleration structure buffer.
		gpu::buffer::create_info ASBCI;
		ASBCI.Memory = device::memory::DEVICE_LOCAL;
		ASBCI.Usage = buffer::usage::ACCELERATION_STRUCTURE_STORAGE_KHR | buffer::usage::SHADER_DEVICE_ADDRESS | buffer::usage::STORAGE | buffer::usage::TRANSFER_SRC | buffer::usage::TRANSFER_DST;
		this->Buffer = aContext->create<buffer>(ASBCI, ASBSI.accelerationStructureSize);

		// TODO: Add support for updating acceleration structures?

		// Scratch buffer is used for temporary storage during acceleration structure build.
		gpu::buffer::create_info SBCI;
		SBCI.Memory = device::memory::DEVICE_LOCAL;
		SBCI.Usage = buffer::usage::SHADER_DEVICE_ADDRESS | buffer::usage::STORAGE | buffer::usage::TRANSFER_SRC | buffer::usage::TRANSFER_DST;
		this->BuildScratchBuffer = aContext->create<buffer>(SBCI, ASBSI.buildScratchSize);

		VkAccelerationStructureCreateInfoKHR ASCI{};
		ASCI.sType 				= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		ASCI.pNext 				= NULL;
		ASCI.createFlags 		= 0;
		ASCI.buffer 			= this->Buffer->Handle;
		ASCI.offset 			= 0;
		ASCI.size 				= ASBSI.accelerationStructureSize;  // Size we got from vkGetAccelerationStructureBuildSizesKHR
		ASCI.type 				= VK_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL_KHR;
		ASCI.deviceAddress 		= 0;  // This is for capturing device address during creation, typically not needed

		VkResult Result = vkCreateAccelerationStructureKHR(aContext->Handle, &ASCI, NULL, &this->Handle);

		if (Result == VK_SUCCESS) {
			// Update ASBGI with the acceleration structure handle.
			ASBGI.dstAccelerationStructure = this->Handle;
			// Update ASBGI with the scratch buffer device address (NOT the AS buffer address).
			ASBGI.scratchData.deviceAddress = this->BuildScratchBuffer->device_address();

			// Now fill out acceleration structure buffer.
			VkAccelerationStructureBuildRangeInfoKHR ASBRI;
			ASBRI.primitiveCount 	= PrimitiveCount;
			ASBRI.primitiveOffset 	= 0;
			ASBRI.firstVertex 		= 0;
			ASBRI.transformOffset 	= 0;

			VkMemoryBarrier Barrier{};
			Barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			Barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			Barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

			auto CommandPool = aContext->create<command_pool>(device::operation::GRAPHICS);
			auto CommandBuffer = CommandPool->create<command_buffer>();

			// Record Command.
			CommandBuffer->begin();
			const VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &ASBRI;
			vkCmdBuildAccelerationStructuresKHR(CommandBuffer->Handle, 1, &ASBGI, &pBuildRangeInfo);
			vkCmdPipelineBarrier(
				CommandBuffer->Handle,
				VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
				0,
				1, &Barrier,
				0, NULL,
				0, NULL
			);
			CommandBuffer->end();

			aContext->execute_and_wait(device::operation::GRAPHICS, CommandBuffer);

			// Get the device address of the acceleration structure.
			this->DeviceAddress = this->device_address();
		}
	}

	acceleration_structure::acceleration_structure(
		std::shared_ptr<context> 									aContext, 
		const std::vector<VkAccelerationStructureInstanceKHR>& 		aInstanceList	// List of Mesh Instances in Scene
	) {
		PFN_vkGetAccelerationStructureBuildSizesKHR vkGetAccelerationStructureBuildSizesKHR = (PFN_vkGetAccelerationStructureBuildSizesKHR)aContext->function_pointer("vkGetAccelerationStructureBuildSizesKHR");
		PFN_vkCreateAccelerationStructureKHR vkCreateAccelerationStructureKHR = (PFN_vkCreateAccelerationStructureKHR)aContext->function_pointer("vkCreateAccelerationStructureKHR");
		PFN_vkCmdBuildAccelerationStructuresKHR vkCmdBuildAccelerationStructuresKHR = (PFN_vkCmdBuildAccelerationStructuresKHR)aContext->function_pointer("vkCmdBuildAccelerationStructuresKHR");
		PFN_vkCmdPipelineBarrier vkCmdPipelineBarrier = (PFN_vkCmdPipelineBarrier)aContext->function_pointer("vkCmdPipelineBarrier");

		this->Context = aContext;

		// Create Instance Buffer for TLAS.
		{
			gpu::buffer::create_info IBCI;
			IBCI.Memory = device::memory::DEVICE_LOCAL;
			IBCI.Usage = buffer::usage::ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_KHR | buffer::usage::ACCELERATION_STRUCTURE_STORAGE_KHR | buffer::usage::SHADER_DEVICE_ADDRESS | buffer::usage::STORAGE | buffer::usage::TRANSFER_SRC | buffer::usage::TRANSFER_DST;
			this->InstanceBuffer = aContext->create<buffer>(IBCI, aInstanceList.size() * sizeof(VkAccelerationStructureInstanceKHR), const_cast<void*>(static_cast<const void*>(aInstanceList.data())));
		}

		// Define geometry for TLAS, similar struct to BLAS.
		VkAccelerationStructureGeometryKHR ASG{};
		ASG.sType												= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_KHR;
		ASG.pNext												= NULL;
		ASG.geometryType										= VK_GEOMETRY_TYPE_INSTANCES_KHR;
		ASG.geometry.instances.sType							= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_GEOMETRY_INSTANCES_DATA_KHR;
		ASG.geometry.instances.pNext							= NULL;
		ASG.geometry.instances.arrayOfPointers					= VK_FALSE;
		ASG.geometry.instances.data.deviceAddress				= this->InstanceBuffer->device_address();
		ASG.flags												= VK_GEOMETRY_OPAQUE_BIT_KHR;

		// Build Geometry Info for TLAS.
		VkAccelerationStructureBuildGeometryInfoKHR ASBGI{};
		ASBGI.sType												= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_GEOMETRY_INFO_KHR;
		ASBGI.pNext												= NULL;
		ASBGI.type												= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		ASBGI.flags												= VK_BUILD_ACCELERATION_STRUCTURE_PREFER_FAST_TRACE_BIT_KHR; // TODO: change to fast build since we update often for mesh anims.
		ASBGI.mode												= VK_BUILD_ACCELERATION_STRUCTURE_MODE_BUILD_KHR;
		ASBGI.srcAccelerationStructure							= VK_NULL_HANDLE;
		ASBGI.dstAccelerationStructure							= VK_NULL_HANDLE;
		ASBGI.geometryCount										= 1;
		ASBGI.pGeometries										= &ASG;
		ASBGI.ppGeometries										= NULL;
		ASBGI.scratchData.deviceAddress							= 0;	

		// Get build sizes based on geometry info provided.
		VkAccelerationStructureBuildSizesInfoKHR ASBSI{};
		ASBSI.sType												= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_BUILD_SIZES_INFO_KHR;
		ASBSI.pNext												= NULL;
		uint32_t PrimitiveCount = aInstanceList.size();
		vkGetAccelerationStructureBuildSizesKHR(aContext->Handle, VK_ACCELERATION_STRUCTURE_BUILD_TYPE_DEVICE_KHR, &ASBGI, &PrimitiveCount, &ASBSI);

		// Build the acceleration structure buffer.
		gpu::buffer::create_info ASBCI;
		ASBCI.Memory = device::memory::DEVICE_LOCAL;
		ASBCI.Usage = buffer::usage::ACCELERATION_STRUCTURE_STORAGE_KHR | buffer::usage::SHADER_DEVICE_ADDRESS | buffer::usage::STORAGE | buffer::usage::TRANSFER_SRC | buffer::usage::TRANSFER_DST;
		this->Buffer = aContext->create<buffer>(ASBCI, ASBSI.accelerationStructureSize);

		// TODO: Add support for updating acceleration structures?

		// Scratch buffer is used for temporary storage during acceleration structure build.
		gpu::buffer::create_info SBCI;
		SBCI.Memory = device::memory::DEVICE_LOCAL;
		SBCI.Usage = buffer::usage::SHADER_DEVICE_ADDRESS | buffer::usage::STORAGE | buffer::usage::TRANSFER_SRC | buffer::usage::TRANSFER_DST;
		this->BuildScratchBuffer = aContext->create<buffer>(SBCI, ASBSI.buildScratchSize);

		VkAccelerationStructureCreateInfoKHR ASCI{};
		ASCI.sType					= VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_CREATE_INFO_KHR;
		ASCI.pNext					= NULL;
		ASCI.createFlags			= 0;
		ASCI.buffer					= this->Buffer->Handle;
		ASCI.offset					= 0;
		ASCI.size					= ASBSI.accelerationStructureSize;
		ASCI.type					= VK_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL_KHR;
		ASCI.deviceAddress			= 0;

		// Create Top Level Acceleration Structure.
		VkResult Result = vkCreateAccelerationStructureKHR(aContext->Handle, &ASCI, NULL, &this->Handle);

		// Now fill out acceleration structure buffer.
		if (Result == VK_SUCCESS) {
			// Update ASBGI with the acceleration structure handle.
			ASBGI.dstAccelerationStructure = this->Handle;
			// Update ASBGI with the scratch buffer device address.
			ASBGI.scratchData.deviceAddress = this->BuildScratchBuffer->device_address();

			// Now fill out acceleration structure buffer.
			VkAccelerationStructureBuildRangeInfoKHR ASBRI;
			ASBRI.primitiveCount 	= aInstanceList.size();
			ASBRI.primitiveOffset 	= 0;
			ASBRI.firstVertex 		= 0;
			ASBRI.transformOffset 	= 0;

			VkMemoryBarrier Barrier{};
			Barrier.sType = VK_STRUCTURE_TYPE_MEMORY_BARRIER;
			Barrier.srcAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR;
			Barrier.dstAccessMask = VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR;

			auto CommandPool = aContext->create<command_pool>(device::operation::GRAPHICS);
			auto CommandBuffer = CommandPool->create<command_buffer>();
			const VkAccelerationStructureBuildRangeInfoKHR* pBuildRangeInfo = &ASBRI;

			// Record Command.
			Result = CommandBuffer->begin();
			vkCmdBuildAccelerationStructuresKHR(CommandBuffer->Handle, 1, &ASBGI, &pBuildRangeInfo);
			vkCmdPipelineBarrier(
				CommandBuffer->Handle,
				VK_PIPELINE_STAGE_ACCELERATION_STRUCTURE_BUILD_BIT_KHR, VK_PIPELINE_STAGE_RAY_TRACING_SHADER_BIT_KHR,
				0,
				1, &Barrier,
				0, NULL,
				0, NULL
			);
			Result = CommandBuffer->end();

			// Execute and wait.
			Result = aContext->execute_and_wait(device::operation::GRAPHICS, CommandBuffer);

			// Get the device address of the acceleration structure.
			this->DeviceAddress = this->device_address();
		}
	}

	acceleration_structure::~acceleration_structure() {

	}

	VkDeviceAddress acceleration_structure::device_address() const {
		// Get AS device address.
		VkAccelerationStructureDeviceAddressInfoKHR ASDAI{};
		ASDAI.sType = VK_STRUCTURE_TYPE_ACCELERATION_STRUCTURE_DEVICE_ADDRESS_INFO_KHR;
		ASDAI.accelerationStructure = this->Handle;
		PFN_vkGetAccelerationStructureDeviceAddressKHR vkGetAccelerationStructureDeviceAddressKHR = (PFN_vkGetAccelerationStructureDeviceAddressKHR)this->Context->function_pointer("vkGetAccelerationStructureDeviceAddressKHR");
		return vkGetAccelerationStructureDeviceAddressKHR(this->Context->Handle, &ASDAI);
	}

}