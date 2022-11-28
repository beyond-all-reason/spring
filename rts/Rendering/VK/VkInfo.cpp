#include "VkInfo.h"
#include "VkInit.h"

#include "System/Log/ILog.h"
#include "fmt/format.h"

#if !defined(HEADLESS) && defined(HAS_VULKAN)
void VkInfo::PrintInfoImpl(const char* funcName)
{
	LOG("[VkInfo::%s]", funcName);

	const auto& vkc = VkCoreObjects::GetInstance();

	LOG("\tInstance VK API version : %s", vkc.GetVulkanAPI().c_str());

	LOG("\tFound the following VK devices:");
	for (const auto& pd : vkc.GetPhysDevices()) {
		const auto& prop = pd.getProperties();
		LOG("\t\t%s (%s)", &prop.deviceName[0], vk::to_string(prop.deviceType).c_str());
	}

	{
		const auto& prop = vkc.GetBestPhysDevice().getProperties();
		LOG("\tSelected best device: %s", &prop.deviceName[0]);
		LOG("\t\tDevice VK API version : %s", vkc.GetVulkanAPI(prop.apiVersion).c_str());
		//prop.limits
	}

	const auto& instanceExtensions = vkc.GetInstanceExtensions();
	const auto& phDeviceExtensions = vkc.GetPhysDeviceExtensions();

	const bool mem2 =
		vkc.HasExtension(instanceExtensions, VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME) &&
		vkc.HasExtension(phDeviceExtensions, VK_EXT_MEMORY_BUDGET_EXTENSION_NAME);

	std::array<uint32_t, 3> heapMemInfo = {0u, 0u, 0u}; //used, budget, total
	if (mem2) {
		const auto memoryProperties2 = vkc.GetBestPhysDevice().getMemoryProperties2<vk::PhysicalDeviceMemoryProperties2, vk::PhysicalDeviceMemoryBudgetPropertiesEXT>();
		const auto& memoryProperties = memoryProperties2.get<vk::PhysicalDeviceMemoryProperties2>().memoryProperties;
		const auto& memoryBudgetProperties = memoryProperties2.get<vk::PhysicalDeviceMemoryBudgetPropertiesEXT>();

		for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
			if (!(memoryProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal))
				continue;

			heapMemInfo[0] += memoryBudgetProperties.heapUsage[i]  / 1024 / 1024;
			heapMemInfo[1] += memoryBudgetProperties.heapBudget[i] / 1024 / 1024;
			heapMemInfo[2] += memoryProperties.memoryHeaps[i].size / 1024 / 1024;
		}
	}
	else {
		const auto memoryProperties = vkc.GetBestPhysDevice().getMemoryProperties();
		for (uint32_t i = 0; i < memoryProperties.memoryHeapCount; ++i) {
			if (!(memoryProperties.memoryHeaps[i].flags & vk::MemoryHeapFlagBits::eDeviceLocal))
				continue;

			heapMemInfo[2] += memoryProperties.memoryHeaps[i].size / 1024 / 1024;
		}
	}

	const std::string memStr = fmt::format("{}/{}/{}",
		heapMemInfo[0] > 0 ? std::to_string(heapMemInfo[0]) : "-",
		heapMemInfo[1] > 0 ? std::to_string(heapMemInfo[1]) : "-",
		heapMemInfo[2]
	);

	LOG("\t\tVRAM : %s MB (used/budget/total)", memStr.c_str());

	static constexpr std::array requiredDeviceExtensions {
		"VK_KHR_dedicated_allocation",
		"VK_KHR_create_renderpass2",
		"VK_KHR_draw_indirect_count",
		"VK_KHR_imageless_framebuffer",
		"VK_KHR_uniform_buffer_standard_layout",
		"VK_EXT_descriptor_indexing",
		"VK_EXT_shader_viewport_index_layer",
		"VK_KHR_dynamic_rendering",
		"VK_KHR_synchronization2",
		"VK_EXT_extended_dynamic_state",
		"VK_EXT_extended_dynamic_state2",
	};

	static constexpr size_t paddingMaxSize = 40;
	for (const auto& rde : requiredDeviceExtensions) {
		const std::string padding(paddingMaxSize - std::string_view(rde).size(), ' ');
		LOG("\t\t%s%s : %i", rde, padding.c_str(), vkc.HasExtension(phDeviceExtensions, rde));
	}

	VkCoreObjects::KillInstance();
}
#else
void VkInfo::PrintInfoImpl(const char* funcName) {}
#endif



void VkInfo::PrintInfo()
{
#ifndef HEADLESS
	#ifndef HAS_VULKAN
		LOG("[VkInfo::%s] Compiled without VULKAN support", __func__);
	#else
		try {
			PrintInfoImpl(__func__);
		} catch (const std::exception& e) {
			LOG("[VkInfo::%s] Runtime error: \"%s\"", __func__, e.what());
		}
	#endif
#endif
}