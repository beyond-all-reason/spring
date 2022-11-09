#ifdef HAS_VULKAN

#include <string_view>

#include "VkInit.h"
#include "fmt/format.h"

bool VkPhysicalDevicesCompare(const vk::PhysicalDevice& lhs, const vk::PhysicalDevice& rhs)
{
	const auto lhsDiscrete = static_cast<uint8_t>(lhs.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu);
	const auto rhsDiscrete = static_cast<uint8_t>(rhs.getProperties().deviceType == vk::PhysicalDeviceType::eDiscreteGpu);

	if (lhsDiscrete > rhsDiscrete) return true;
	if (lhsDiscrete < rhsDiscrete) return false;

	VkDeviceSize lhsMS = VkCoreObjects::GetVRAMSize(lhs);
	VkDeviceSize rhsMS = VkCoreObjects::GetVRAMSize(rhs);

	if (lhsMS > rhsMS) return true;
	if (lhsMS < rhsMS) return false;

	return false;
};

VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE
VkCoreObjects::VkCoreObjects()
{
	dl = std::make_unique<vk::DynamicLoader>();

	PFN_vkGetInstanceProcAddr vkGetInstanceProcAddr = dl->getProcAddress<PFN_vkGetInstanceProcAddr>("vkGetInstanceProcAddr");
	VULKAN_HPP_DEFAULT_DISPATCHER.init(vkGetInstanceProcAddr);

	// Determine what API version is available
	apiVersion = vk::enumerateInstanceVersion();

	vk::ApplicationInfo applicationInfo("spring", 1, "spring", 1, apiVersion);
	vk::InstanceCreateInfo instanceCreateInfo({}, &applicationInfo);
	instance = vk::createInstance(instanceCreateInfo);
	VULKAN_HPP_DEFAULT_DISPATCHER.init(instance);

	instExtensionProperties = vk::enumerateInstanceExtensionProperties();

	physicalDevices = instance.enumeratePhysicalDevices();
	std::sort(physicalDevices.begin(), physicalDevices.end(), VkPhysicalDevicesCompare);

	physicalDevice = physicalDevices.front();
	pdevExtensionProperties = physicalDevice.enumerateDeviceExtensionProperties();
}

std::string VkCoreObjects::GetVulkanAPI(uint32_t apiVersion)
{
	return fmt::format("{}.{}.{}", VK_API_VERSION_MAJOR(apiVersion), VK_API_VERSION_MINOR(apiVersion), VK_API_VERSION_PATCH(apiVersion));
}

std::string VkCoreObjects::GetVulkanAPI() const
{
	return GetVulkanAPI(apiVersion);
}

VkDeviceSize VkCoreObjects::GetVRAMSize(const vk::PhysicalDevice& phd)
{
	VkDeviceSize memSize = 0;
	const auto memProp = phd.getMemoryProperties();
	for (auto hi = 0; hi < memProp.memoryHeapCount; ++hi) {
		if (memProp.memoryHeaps[hi].flags & vk::MemoryHeapFlagBits::eDeviceLocal)
			memSize += memProp.memoryHeaps[hi].size;
	}

	return memSize;
}

bool VkCoreObjects::HasExtension(const std::vector<vk::ExtensionProperties>& es, const char* e)
{
	return std::find_if(es.begin(), es.end(),
		[e](const auto& item) {
			return std::string_view(item.extensionName).compare(e) == 0;
		}
	) != es.end();
}

VkCoreObjects::~VkCoreObjects()
{
	instance.destroy();
}

#endif