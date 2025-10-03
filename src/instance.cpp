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
		void* 						avkGetInstanceProcAddr,
		std::array<int, 3> 			aAPIVersion,
		std::set<std::string> 		aLayers, 
		std::set<std::string> 		aExtensions,
		void* 						aNext,
		std::string 				aAppName,
		std::array<int, 3> 			aAppVersion,
		std::string 				aEngineName,
		std::array<int, 3> 			aEngineVersion
	) : instance() {
		// Determine if loading function has been provided or not. If yes, use it to access all other vulkan functions.
		this->vkGetInstanceProcAddr = (PFN_vkGetInstanceProcAddr)avkGetInstanceProcAddr;// ? (PFN_vkGetInstanceProcAddr)avkGetInstanceProcAddr : ::vkGetInstanceProcAddr;
		
		// Load necessary function pointers onto stack.
		PFN_vkCreateInstance vkCreateInstance = (PFN_vkCreateInstance)this->function_pointer("vkCreateInstance");
		PFN_vkEnumeratePhysicalDevices vkEnumeratePhysicalDevices = (PFN_vkEnumeratePhysicalDevices)this->function_pointer("vkEnumeratePhysicalDevices");

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
		ICI.pNext							= aNext;
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

		uint32_t PhysicalDeviceCount = 0;
		Result = vkEnumeratePhysicalDevices(this->Handle, &PhysicalDeviceCount, NULL);
		if (PhysicalDeviceCount > 0) {
			std::vector<VkPhysicalDevice> PhysicalDeviceList(PhysicalDeviceCount);
			Result = vkEnumeratePhysicalDevices(this->Handle, &PhysicalDeviceCount, PhysicalDeviceList.data());
			if (Result != VK_SUCCESS) {
				throw std::runtime_error("Failed to enumerate Vulkan physical devices.");
			}

			// Create device list.
			for (const auto& PhysicalDevice : PhysicalDeviceList) {
				auto NewDevice = geodesy::make<gpu::device>(this, PhysicalDevice);
				if (NewDevice) {
					this->Device.push_back(NewDevice);
				}
			}
		}
	}
	
	instance::~instance() {
		PFN_vkDestroyInstance vkDestroyInstance = (PFN_vkDestroyInstance)this->function_pointer("vkDestroyInstance");
		// Destroy device objects first.
		this->Device.clear();
		// Then destroy instance.
		vkDestroyInstance(this->Handle, NULL);
		this->Handle = VK_NULL_HANDLE;
	}

	void* instance::function_pointer(std::string aFunctionName) const {
		return vkGetInstanceProcAddr(this->Handle, aFunctionName.c_str());
	}
	
	std::vector<std::shared_ptr<gpu::device>> instance::get_devices() {
		return this->Device;
	}

	std::shared_ptr<gpu::context> instance::create_context(
		std::shared_ptr<device> 		aDevice,
		std::vector<unsigned int> 		aExecutionOperations,
		std::set<std::string> 			aLayers,
		std::set<std::string> 			aExtensions,
		void* 							aNext
	) {
		return geodesy::make<gpu::context>(
			this->shared_from_this(),
			aDevice,
			aExecutionOperations,
			aLayers,
			aExtensions,
			aNext
		);
	}
	
}
