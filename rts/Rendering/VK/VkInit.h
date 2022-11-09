#pragma once
#ifdef HAS_VULKAN

#include <vector>
#include <memory>

#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1
#include <vulkan/vulkan.hpp>

class VkCoreObjects {
public:
	VkCoreObjects();
	~VkCoreObjects();

	const auto& GetInstanceExtensions() const { return instExtensionProperties; }
	const auto& GetPhysDeviceExtensions() const { return pdevExtensionProperties; }
	const auto& GetPhysDevices() const { return physicalDevices; }
	const auto& GetBestPhysDevice() const { return physicalDevice; }

	std::string GetVulkanAPI() const;
	static std::string GetVulkanAPI(uint32_t apiVersion);

	static VkDeviceSize GetVRAMSize(const vk::PhysicalDevice& phd);
	static bool HasExtension(const std::vector<vk::ExtensionProperties>& es, const char* e);

	static VkCoreObjects& GetInstance() {
		if (!vkCoreObjects) {
			vkCoreObjects = std::make_unique<VkCoreObjects>();
		}
		return *vkCoreObjects;
	}
	static void KillInstance() { vkCoreObjects = nullptr; }
private:
	static inline std::unique_ptr<VkCoreObjects> vkCoreObjects = nullptr;

	std::unique_ptr<vk::DynamicLoader> dl;

	vk::Instance instance;
	std::vector<vk::ExtensionProperties> instExtensionProperties;
	std::vector<vk::ExtensionProperties> pdevExtensionProperties;

	std::vector<vk::PhysicalDevice> physicalDevices;
	vk::PhysicalDevice physicalDevice;

	uint32_t apiVersion;

	//static constexpr 
};

#endif