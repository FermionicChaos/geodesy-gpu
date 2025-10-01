#ifndef GEODESY_GPU_CONFIG_H
#define GEODESY_GPU_CONFIG_H

#include <vector>
#include <set>
#include <memory>

#include <vulkan/vulkan.h>

namespace geodesy {

	// Template function to create shared_ptr with perfect forwarding
	template<typename T, typename... Args>
	std::shared_ptr<T> make(Args&&... args) {
		return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
	}

	namespace gpu {
		class instance;
		class context;
	}
	
}

#endif // !GEODESY_GPU_CONFIG_H
