#ifndef GEODESY_GPU_INSTANCE_H
#define GEODESY_GPU_INSTANCE_H

#include "config.h"

#include "device.h"

namespace geodesy::gpu {

	class instance : public std::enable_shared_from_this<instance> {
	public:

		static std::set<std::string> SupportedLayers;
		static std::set<std::string> SupportedExtensions;

		static bool initialize();
		static void terminate();

		VkInstance Handle;

		instance();
		instance(
			std::set<std::string> 		aLayers, 
			std::set<std::string> 		aExtensions,
			std::array<int, 3> 			aAPIVersion = { 1, 0, 0 },
			std::string 				aAppName = "",
			std::array<int, 3> 			aAppVersion = { 1, 0, 0 },
			std::string 				aEngineName = "",
			std::array<int, 3> 			aEngineVersion = { 1, 0, 0 }
		);
		~instance();

		void* function_pointer(std::string aFunctionName);

		std::vector<std::shared_ptr<gpu::device>> get_devices();

	private:

		PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr;
		std::vector<std::shared_ptr<gpu::device>> Device;

	};

}

#endif // !GEODESY_GPU_INSTANCE_H
