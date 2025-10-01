#ifndef GEODESY_GPU_CONFIG_H
#define GEODESY_GPU_CONFIG_H

#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <set>
#include <map>
#include <memory>

#include <vulkan/vulkan.h>

namespace geodesy {

	// Template function to create shared_ptr with perfect forwarding
	template <typename T, typename... Args>
	std::shared_ptr<T> make(Args&&... args) {
		try {
			return std::shared_ptr<T>(new T(std::forward<Args>(args)...));
		} catch (...) {
			return nullptr;
		}
	}

	namespace gpu {
		class context;
		class instance;
	}
	
}

#endif // !GEODESY_GPU_CONFIG_H
