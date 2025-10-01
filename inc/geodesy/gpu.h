#ifndef GEODESY_GPU_H
#define GEODESY_GPU_H

// Configuration for library
#include "gpu/config.h"

#include "gpu/device.h"
#include "gpu/resource.h"
// ----- Command Buffer Management ----- //
// Execution and Synchronization Primitives
// Explanation: this section of code manages command buffers and command pools.
// Since command buffers record and reference both gpu resources and meta resources, they
// must be deleted first before anything else. 
#include "gpu/fence.h"
#include "gpu/semaphore.h"
#include "gpu/semaphore_pool.h"
#include "gpu/command_buffer.h"
#include "gpu/command_pool.h"
#include "gpu/command_batch.h"
// ----- GPU Resource Types ----- //
// Explanation of resource types:
//      - A resource is a handle to a GPU object.
//      - A resource requires a context to be created.
//      - A resource can be a buffer, an image, a shader, or a pipeline.
// GPU Resources are hard data such as vertex buffers, index buffers, uniform buffers,
// textures, and image outputs for rendering.
// #include "gpu/buffer.h"
// #include "gpu/image.h"
// #include "gpu/shader.h" // Not Actually GPU Resource, no context required for creation.
// #include "gpu/pipeline.h"
// ----- GPU Metaresources ----- //
// Explanation of meta resources:
//      - A Descriptor Set is a reference of gpu buffers & images.
//      - A framebuffer is a reference of gpu images.
//      - An acceleration structure is a reference of gpu buffer geometries and transforms..
// Metaresources are references to other gpu resources in existence. Their lifetime is bound
// to the life times of the objects they were created with. They must always be cleared before
// the objects they reference are destroyed.
// #include "gpu/descriptor.h"
// #include "gpu/framebuffer.h"
// #include "gpu/acceleration_structure.h"
// Stuff goes here...
#include "gpu/context.h"
#include "gpu/instance.h"

#endif // !GEODESY_GPU_H