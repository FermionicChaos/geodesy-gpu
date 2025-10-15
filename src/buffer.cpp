#include <geodesy/gpu/buffer.h>

#include <cstdlib>
#include <cstring>
#include <assert.h>

#include <vector>
#include <algorithm>

#include <geodesy/gpu/image.h>
#include <geodesy/gpu/context.h>

// It approximately 16 MB
#define GPU_TRANSFER_GRANULARITY_SIZE (1 << 24)

namespace geodesy::gpu {

	buffer::create_info::create_info() {
		this->Memory 	= (device::memory)0u;
		this->Usage 	= (buffer::usage)0u;
		this->ElementCount = 1;
	}

	buffer::create_info::create_info(unsigned int aMemoryType, unsigned int aBufferUsage) : create_info() {
		this->Memory = aMemoryType;
		this->Usage = aBufferUsage;
	}

	buffer::buffer() {
		this->Context		= nullptr;
		this->Type			= resource::type::BUFFER;
		this->CreateInfo 	= {};
		this->Handle 		= VK_NULL_HANDLE;
		this->ElementCount 	= 0;
		this->MemoryType 	= 0;
		this->MemoryHandle 	= VK_NULL_HANDLE;
		this->Ptr 			= NULL;
	}

	buffer::buffer(std::shared_ptr<context> aContext, create_info aCreateInfo, size_t aBufferSize, void* aBufferData) 
	: buffer(aContext, aCreateInfo.Memory, aCreateInfo.Usage, aCreateInfo.ElementCount, aBufferSize, aBufferData) {}

	buffer::buffer(std::shared_ptr<context> aContext, unsigned int aMemoryType, unsigned int aBufferUsage, size_t aBufferSize, void* aBufferData)
	: buffer(aContext, aMemoryType, aBufferUsage, 1, aBufferSize, aBufferData) {}

	buffer::buffer(std::shared_ptr<context> aContext, unsigned int aMemoryType, unsigned int aBufferUsage, size_t aElementCount, size_t aBufferSize, void* aBufferData) : buffer() {
		VkResult Result = VK_SUCCESS;
		PFN_vkCreateBuffer vkCreateBuffer = (PFN_vkCreateBuffer)aContext->function_pointer("vkCreateBuffer");
		PFN_vkBindBufferMemory vkBindBufferMemory = (PFN_vkBindBufferMemory)aContext->function_pointer("vkBindBufferMemory");

		Context 								= aContext;
		ElementCount							= aElementCount;

		CreateInfo.sType						= VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
		CreateInfo.pNext						= NULL;
		CreateInfo.flags						= 0;
		CreateInfo.size							= aBufferSize;
		CreateInfo.usage						= (VkBufferUsageFlags)(aBufferUsage);
		CreateInfo.sharingMode					= VK_SHARING_MODE_EXCLUSIVE;
		CreateInfo.queueFamilyIndexCount		= 0;
		CreateInfo.pQueueFamilyIndices			= NULL;

		if (Context != nullptr) {
			// Create Buffer Object
			Result = vkCreateBuffer(Context->Handle, &CreateInfo, NULL, &Handle);

			// Get Memory Requirements for Buffer.
			VkMemoryRequirements MemoryRequirements = this->memory_requirements();

			// Allocate memory for buffer.
			MemoryType = aMemoryType;
			MemoryHandle = Context->allocate_memory(MemoryRequirements, aMemoryType);

			// Bind Buffer to allocated Memory.
			Result = vkBindBufferMemory(this->Context->Handle, this->Handle, this->MemoryHandle, 0);

			// Write data to buffer object.
			if (aBufferData != NULL) {
				this->write(0, aBufferData, 0, aBufferSize);
			}
		}
	}

	buffer::~buffer() {
		PFN_vkDestroyBuffer vkDestroyBuffer = (PFN_vkDestroyBuffer)this->Context->function_pointer("vkDestroyBuffer");
		// Unmap memory if mapped.
		this->unmap_memory();
		// Destroy buffer object.
		vkDestroyBuffer(this->Context->Handle, this->Handle, NULL);
		// Free allocated memory.
		this->Context->free_memory(this->MemoryHandle);
	}

	void buffer::copy(command_buffer* aCommandBuffer, size_t aDestinationOffset, std::shared_ptr<buffer> aSourceData, size_t aSourceOffset, size_t aRegionSize) {
		VkBufferCopy Region{};
		Region.srcOffset		= aSourceOffset;
		Region.dstOffset		= aDestinationOffset;
		Region.size				= aRegionSize;
		std::vector<VkBufferCopy> RegionList;
		RegionList.push_back(Region);
		this->copy(aCommandBuffer, aSourceData, RegionList);
	}

	void buffer::copy(command_buffer* aCommandBuffer, std::shared_ptr<buffer> aSourceData, std::vector<VkBufferCopy> aRegionList) {
		PFN_vkCmdCopyBuffer vkCmdCopyBuffer = (PFN_vkCmdCopyBuffer)this->Context->function_pointer("vkCmdCopyBuffer");
		vkCmdCopyBuffer(aCommandBuffer->Handle, aSourceData->Handle, this->Handle, aRegionList.size(), aRegionList.data());
	}

	void buffer::copy(command_buffer* aCommandBuffer, size_t aDestinationOffset, std::shared_ptr<image> aSourceData, VkOffset3D aSourceOffset, uint32_t aSourceArrayLayer, VkExtent3D aRegionExtent, uint32_t aArrayLayerCount) {
		VkBufferImageCopy Region {};
		Region.bufferOffset						= aDestinationOffset;
		Region.bufferRowLength					= 0;
		Region.bufferImageHeight				= 0;
		Region.imageSubresource.aspectMask		= image::aspect_flag(aSourceData->CreateInfo.format);
		Region.imageSubresource.mipLevel		= 0;
		Region.imageSubresource.baseArrayLayer	= aSourceArrayLayer;
		Region.imageSubresource.layerCount		= std::min(aArrayLayerCount, aSourceData->CreateInfo.arrayLayers - aSourceArrayLayer);
		std::vector<VkBufferImageCopy> RegionList = { Region };
		this->copy(aCommandBuffer, aSourceData, RegionList);
	}

	void buffer::copy(command_buffer* aCommandBuffer, std::shared_ptr<image> aSourceData, std::vector<VkBufferImageCopy> aRegionList) {
		PFN_vkCmdCopyImageToBuffer vkCmdCopyImageToBuffer = (PFN_vkCmdCopyImageToBuffer)this->Context->function_pointer("vkCmdCopyImageToBuffer");
		vkCmdCopyImageToBuffer(
			aCommandBuffer->Handle, 
			aSourceData->Handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
			this->Handle, 
			aRegionList.size(), aRegionList.data()
		);
	}
	
	VkResult buffer::copy(size_t aDestinationOffset, std::shared_ptr<buffer> aSourceData, size_t aSourceOffset, size_t aRegionSize) {
		VkBufferCopy Region{};
		Region.srcOffset		= aSourceOffset;
		Region.dstOffset		= aDestinationOffset;
		Region.size				= aRegionSize;
		std::vector<VkBufferCopy> RegionList;
		RegionList.push_back(Region);
		return this->copy(aSourceData, RegionList);
	}

	VkResult buffer::copy(std::shared_ptr<buffer> aSourceData, std::vector<VkBufferCopy> aRegionList) {
		VkResult Result = VK_SUCCESS;
		auto CommandPool = Context->create<command_pool>(device::operation::TRANSFER);
		auto CommandBuffer = CommandPool->allocate_command_buffer();

		// Record Command Buffer
		Result = CommandBuffer->begin();
		this->copy(CommandBuffer.get(), aSourceData, aRegionList);
		Result = CommandBuffer->end();

		// Execute command buffer.
		Result = Context->execute_and_wait(device::operation::TRANSFER, CommandBuffer);

		return Result;
	}

	VkResult buffer::copy(size_t aDestinationOffset, std::shared_ptr<image> aSourceData, VkOffset3D aSourceOffset, uint32_t aSourceArrayLayer, VkExtent3D aRegionExtent, uint32_t aArrayLayerCount) {
		VkBufferImageCopy Region{};
		Region.bufferOffset						= aDestinationOffset;
		Region.bufferRowLength					= 0;
		Region.bufferImageHeight				= 0;
		Region.imageSubresource.aspectMask		= image::aspect_flag(aSourceData->CreateInfo.format);
		Region.imageSubresource.mipLevel		= 0;
		Region.imageSubresource.baseArrayLayer	= aSourceArrayLayer;
		Region.imageSubresource.layerCount		= std::min(aArrayLayerCount, aSourceData->CreateInfo.arrayLayers - aSourceArrayLayer);
		Region.imageOffset 						= aSourceOffset;
		Region.imageExtent 						= aRegionExtent;
		std::vector<VkBufferImageCopy> RegionList = { Region };
		return this->copy(aSourceData, RegionList);
	}

	VkResult buffer::copy(std::shared_ptr<image> aSourceData, std::vector<VkBufferImageCopy> aRegionList) {
		VkResult Result = VK_SUCCESS;
		auto CommandPool = Context->create<command_pool>(device::operation::TRANSFER);
		auto CommandBuffer = CommandPool->allocate_command_buffer();

		// Record Command Buffer
		Result = CommandBuffer->begin();
		this->copy(CommandBuffer.get(), aSourceData, aRegionList);
		Result = CommandBuffer->end();

		// Execute command buffer.
		Result = Context->execute_and_wait(device::operation::TRANSFER, CommandBuffer);

		return Result;
	}

	VkResult buffer::write(size_t aDestinationOffset, void* aSourceData, size_t aSourceOffset, size_t aRegionSize) {
		std::vector<VkBufferCopy> RegionList;
		VkBufferCopy Region{ aSourceOffset, aDestinationOffset, aRegionSize };
		RegionList.push_back(Region);
		return this->write(aSourceData, RegionList);
	}

	VkResult buffer::write(void* aSourceData, std::vector<VkBufferCopy> aRegionList) {
		VkResult Result = VK_SUCCESS;
		if ((this->MemoryType & device::memory::HOST_VISIBLE) == device::memory::HOST_VISIBLE) {
			// Host Visible, can be written to directly.
			for (size_t i = 0; i < aRegionList.size(); i++) {
				// Calculate offset addresses
				void* ptr = this->map_memory(aRegionList[i].dstOffset, aRegionList[i].size);
				//uintptr_t TargetAddress = (uintptr_t)ptr + aRegionList[i].dstOffset;
				uintptr_t SourceAddress = (uintptr_t)aSourceData + aRegionList[i].srcOffset;
				// Copy specified targets.
				memcpy((void*)ptr, (void*)SourceAddress, aRegionList[i].size);
				this->unmap_memory();
			}
		} else {
			// Not Host Visible, use staging buffer. For large uploads, we will try something different.
			// Say we have a large model, we want to use a staging buffer that is small to upload chunks
			// to device memory.
			size_t StagingBufferSize = GPU_TRANSFER_GRANULARITY_SIZE;

			// Not Host Visible, use staging buffer.
			std::shared_ptr<buffer> StagingBuffer = std::make_shared<buffer>(
				Context,
				device::memory::HOST_VISIBLE | device::memory::HOST_COHERENT,
				buffer::TRANSFER_SRC,
				StagingBufferSize
			);

			// Copy each memory region to target buffer.
			for (size_t i = 0; i < aRegionList.size(); i++) {
				
				size_t Remainder = aRegionList[i].size;
				do {

					// Calculate chunk offsets for region for transfer.
					size_t ChunkOffset = aRegionList[i].size - Remainder;

					// Insure that we do not exceed the staging buffer size.
					size_t ChunkSize = std::clamp(Remainder, (size_t)0, StagingBufferSize);

					// Write data to staging buffer, if less than ChunkSize, then we are done.
					Result = StagingBuffer->write(0, aSourceData, aRegionList[i].srcOffset + ChunkOffset, ChunkSize);

					// Execute transfer operation.
					Result = this->copy(aRegionList[i].dstOffset + ChunkOffset, StagingBuffer, 0, ChunkSize);

					// Recalculate remaining data to send.
					Remainder -= ChunkSize;

				} while (Remainder > 0);

			}

		}
		return Result;
	}

	VkResult buffer::read(size_t aSourceOffset, void* aDestinationData, size_t aDestinationOffset, size_t aRegionSize) {
		std::vector<VkBufferCopy> RegionList;
		VkBufferCopy Region{ aSourceOffset, aDestinationOffset, aRegionSize };
		RegionList.push_back(Region);
		return this->read(aDestinationData, RegionList);
	}

	VkResult buffer::read(void* aDestinationData, std::vector<VkBufferCopy> aRegionList) {
		VkResult Result = VK_SUCCESS;
		if ((this->MemoryType & device::memory::HOST_VISIBLE) == device::memory::HOST_VISIBLE) {
			// Host Visible, can be written to directly.
			for (size_t i = 0; i < aRegionList.size(); i++) {
				// Calculate offset addresses
				void* ptr = this->map_memory(aRegionList[i].srcOffset, aRegionList[i].size);
				uintptr_t TargetAddress = (uintptr_t)aDestinationData + aRegionList[i].dstOffset;
				// Copy specified targets.
				memcpy((void*)TargetAddress, ptr, aRegionList[i].size);
				this->unmap_memory();
			}
		} else {
			// Not Host Visible, use staging buffer. For large uploads, we will try something different.
			// Say we have a large model, we want to use a staging buffer that is small to upload chunks
			// to device memory.
			size_t StagingBufferSize = GPU_TRANSFER_GRANULARITY_SIZE;

			// Not Host Visible, use staging buffer.
			std::shared_ptr<buffer> StagingBuffer = std::make_shared<buffer>(
				Context,
				device::memory::HOST_VISIBLE | device::memory::HOST_COHERENT,
				buffer::TRANSFER_DST,
				StagingBufferSize
			);

			// Copy each memory region to aDestinationData.
			for (size_t i = 0; i < aRegionList.size(); i++) {
				
				size_t Remainder = aRegionList[i].size;
				do {

					// Calculate chunk offsets for region for transfer.
					size_t ChunkOffset = aRegionList[i].size - Remainder;

					// Insure that we do not exceed the staging buffer size.
					size_t ChunkSize = std::clamp(Remainder, (size_t)0, StagingBufferSize);

					// Copy *this into staging buffer.
					StagingBuffer->copy(0, this->shared_from_this(), aRegionList[i].srcOffset + ChunkOffset, ChunkSize);

					// Read staging buffer and copy into host memory aDestination Data.
					StagingBuffer->read(0, aDestinationData, aRegionList[i].dstOffset + ChunkOffset, ChunkSize);

					// Recalculate remaining data to send.
					Remainder -= ChunkSize;

				} while (Remainder > 0);

			}

		}

		return Result;
	}

	void *buffer::map_memory(size_t aOffset, size_t aSize) {
		VkResult Result = VK_SUCCESS;
		PFN_vkMapMemory vkMapMemory = (PFN_vkMapMemory)this->Context->function_pointer("vkMapMemory");
		Result = vkMapMemory(this->Context->Handle, this->MemoryHandle, aOffset, aSize, 0, &this->Ptr);
		return this->Ptr;
	}

	void buffer::unmap_memory() {
		if (this->Ptr != NULL) {
			PFN_vkUnmapMemory vkUnmapMemory = (PFN_vkUnmapMemory)this->Context->function_pointer("vkUnmapMemory");
			vkUnmapMemory(this->Context->Handle, this->MemoryHandle);
			this->Ptr = NULL;
		}
	}

	VkDeviceAddress buffer::device_address() const {
		VkBufferDeviceAddressInfo BDIA{};
		PFN_vkGetBufferDeviceAddress vkGetBufferDeviceAddress = (PFN_vkGetBufferDeviceAddress)this->Context->function_pointer("vkGetBufferDeviceAddress");
		BDIA.sType = VK_STRUCTURE_TYPE_BUFFER_DEVICE_ADDRESS_INFO;
		BDIA.pNext = NULL;
		BDIA.buffer = this->Handle;
		return vkGetBufferDeviceAddress(this->Context->Handle, &BDIA);
	}

	VkBufferMemoryBarrier buffer::memory_barrier(
		unsigned int aSrcAccess, unsigned int aDstAccess,
		size_t aOffset, size_t aSize
	) const {
		VkBufferMemoryBarrier MemoryBarrier{};
		MemoryBarrier.sType						= VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER;
		MemoryBarrier.pNext						= NULL;
		MemoryBarrier.srcAccessMask				= aSrcAccess;
		MemoryBarrier.dstAccessMask				= aDstAccess;
		MemoryBarrier.srcQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
		MemoryBarrier.dstQueueFamilyIndex		= VK_QUEUE_FAMILY_IGNORED;
		MemoryBarrier.buffer					= this->Handle;
		MemoryBarrier.offset					= aOffset;
		MemoryBarrier.size						= std::min(aSize, this->CreateInfo.size - aOffset);
		return MemoryBarrier;
	}

	VkMemoryRequirements buffer::memory_requirements() const {
		return this->Context->get_buffer_memory_requirements(this->Handle);
	}

}
