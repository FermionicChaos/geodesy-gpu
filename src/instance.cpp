#include <geodesy/gpu/instance.h>

namespace geodesy::gpu {

	bool instance::initialize() {
		return true;
	}

	void instance::terminate() {

	}

	instance::instance() {
		this->Handle = VK_NULL_HANDLE;
		this->vkGetInstanceProcAddr = nullptr;
	}

	instance::instance(
		std::set<std::string> 		aLayers, 
		std::set<std::string> 		aExtensions,
		std::array<int, 3> 			aAPIVersion,
		std::string 				aAppName,
		std::array<int, 3> 			aAppVersion,
		std::string 				aEngineName,
		std::array<int, 3> 			aEngineVersion
	) : instance() {

		std::vector<const char*> LayerList;
		for (const auto& Layer : aLayers) {
			LayerList.push_back(Layer.c_str());
		}

		std::vector<const char*> ExtensionList;
		for (const auto& Extension : aExtensions) {
			ExtensionList.push_back(Extension.c_str());
		}

		VkApplicationInfo AI = {};
		AI.sType							= VK_STRUCTURE_TYPE_APPLICATION_INFO;
		AI.pNext							= NULL;
		AI.pApplicationName					= aAppName.empty() ? NULL : aAppName.c_str();
		AI.applicationVersion				= VK_MAKE_VERSION(aAppVersion[0], aAppVersion[1], aAppVersion[2]);
		AI.pEngineName						= aEngineName.empty() ? NULL : aEngineName.c_str();
		AI.engineVersion					= VK_MAKE_VERSION(aEngineVersion[0], aEngineVersion[1], aEngineVersion[2]);
		AI.apiVersion						= VK_MAKE_API_VERSION(aAPIVersion[0], aAPIVersion[1], aAPIVersion[2], 0);

		VkInstanceCreateInfo ICI = {};
		ICI.sType							= VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		ICI.pNext							= NULL;
		ICI.flags							= 0;
		ICI.pApplicationInfo				= &AI;
		ICI.enabledLayerCount				= LayerList.size();
		ICI.ppEnabledLayerNames				= LayerList.data();
		ICI.enabledExtensionCount			= ExtensionList.size();
		ICI.ppEnabledExtensionNames			= ExtensionList.data();

		VkResult Result = vkCreateInstance(&ICI, NULL, &this->Handle);
		if (Result != VK_SUCCESS) {
			throw std::runtime_error("Failed to create Vulkan instance.");
		}
	}
	
	instance::~instance() {
		vkDestroyInstance(this->Handle, NULL);
		this->Handle = VK_NULL_HANDLE;
	}
	
}
