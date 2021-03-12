#include "VulkanDevice.h"

[[ noreturn ]] void VulkanDevice::create_physical_device()
{
	uint32_t physical_devices_count{};
	vkEnumeratePhysicalDevices(instance, &physical_devices_count, nullptr);
	if (!physical_devices_count)
		throw std::runtime_error("Cannot find any GPU with Vulkan support!\n");
	std::vector<VkPhysicalDevice> physical_devices(physical_devices_count);
	vkEnumeratePhysicalDevices(instance, &physical_devices_count, physical_devices.data());
	for (const auto& device : physical_devices)
	{
		if (is_device_suitable(device))
		{
			physical_device = device;
			msaa_samples = get_max_usable_sample_count();
			break;
		}
	}
	if (physical_device == VK_NULL_HANDLE)
		throw std::runtime_error("Cannot find any suitable device!\n");
}

bool VulkanDevice::check_device_extension_support(const VkPhysicalDevice& dev)
{
	uint32_t extension_count{};
	vkEnumerateDeviceExtensionProperties(dev, nullptr, &extension_count, nullptr);
	std::vector<VkExtensionProperties> available_ext(extension_count);
	vkEnumerateDeviceExtensionProperties(dev, nullptr, &extension_count, available_ext.data());
	std::set<std::string> required_extensions(device_extensions.begin(), device_extensions.end());
	for (const auto& ext : available_ext)
		required_extensions.erase(ext.extensionName);
	return required_extensions.empty();
}

bool VulkanDevice::is_device_suitable(const VkPhysicalDevice& dev)
{
	VkPhysicalDeviceProperties device_properties;
	VkPhysicalDeviceFeatures device_features;
	vkGetPhysicalDeviceProperties(dev, &device_properties);
	vkGetPhysicalDeviceFeatures(dev, &device_features);
	Queue_family_indecies indecies = find_queue_family_indicies(dev);
	bool extension_support = check_device_extension_support(dev), swap_chain_adequate = false;
	if (extension_support)
	{
		Swap_chain_support_details det = query_swap_chain_support(dev);
		swap_chain_adequate = !det.formats.empty() && !det.presentation_modes.empty();
	}
	return device_properties.deviceType == /*VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU && device_features.geometryShader &&*/
		indecies.is_complete() && extension_support && swap_chain_adequate && device_features.samplerAnisotropy;
}

[ [ noreturn ] ] void VulkanDevice::create_device(bool enable_validation_layers, const std::vector<const char*>& validation_layers, VkQueue& graphics_queue, VkQueue& present_queue)
{
	std::vector<VkDeviceQueueCreateInfo> queue_info_vec{};
	Queue_family_indecies ind = find_queue_family_indicies(physical_device);
	std::set<uint32_t> queue_indecies{ ind.graphics_family.value(), ind.present_family.value() };
	float queue_priority = 1.0f;
	for (uint32_t queue : queue_indecies)
	{
		VkDeviceQueueCreateInfo queue_info{};
		queue_info.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info.queueFamilyIndex = queue;
		queue_info.queueCount = 1;
		queue_info.pQueuePriorities = &queue_priority;
		queue_info_vec.emplace_back(queue_info);
	}
	VkPhysicalDeviceFeatures device_features{};
	device_features.samplerAnisotropy = device_features.sampleRateShading = device_features.wideLines = device_features.fillModeNonSolid = VK_TRUE;
	VkDeviceCreateInfo logical_device_create_info{};
	logical_device_create_info.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	logical_device_create_info.pQueueCreateInfos = queue_info_vec.data();
	logical_device_create_info.queueCreateInfoCount = static_cast<uint32_t>(queue_info_vec.size());
	logical_device_create_info.pEnabledFeatures = &device_features;
	logical_device_create_info.enabledExtensionCount = static_cast<uint32_t>(device_extensions.size());
	logical_device_create_info.ppEnabledExtensionNames = device_extensions.data();
	if (enable_validation_layers)
	{
		logical_device_create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
		logical_device_create_info.ppEnabledLayerNames = validation_layers.data();
	}
	if (vkCreateDevice(physical_device, &logical_device_create_info, nullptr, &device) != VK_SUCCESS)
		throw std::runtime_error("Failed to create logical device!\n");
	vkGetDeviceQueue(device, ind.graphics_family.value(), 0, &graphics_queue);
	vkGetDeviceQueue(device, ind.present_family.value(), 0, &present_queue);
}

Queue_family_indecies VulkanDevice::find_queue_family_indicies(const VkPhysicalDevice& dev)
{
	Queue_family_indecies indecies{};
	uint32_t family_count{};
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, nullptr);
	std::vector<VkQueueFamilyProperties> queue_families(family_count);
	vkGetPhysicalDeviceQueueFamilyProperties(dev, &family_count, queue_families.data());
	for (int i = 0; i < queue_families.size(); ++i)
	{
		if (queue_families.at(i).queueFlags & VK_QUEUE_GRAPHICS_BIT)
			indecies.graphics_family = i;
		VkBool32 present_family = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(dev, i, surface, &present_family);
		if (present_family)
			indecies.present_family = i;
		if (indecies.is_complete())
			break;
	}

	return indecies;
}

Swap_chain_support_details VulkanDevice::query_swap_chain_support(const VkPhysicalDevice& dev)
{
	Swap_chain_support_details details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(dev, surface, &details.capabilities);
	uint32_t format_count{}, presentation_mode_count{};
	vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, nullptr);
	if (format_count)
	{
		details.formats.resize(format_count);
		vkGetPhysicalDeviceSurfaceFormatsKHR(dev, surface, &format_count, details.formats.data());

	}
	vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentation_mode_count, nullptr);
	if (presentation_mode_count)
	{
		details.presentation_modes.resize(presentation_mode_count);
		vkGetPhysicalDeviceSurfacePresentModesKHR(dev, surface, &presentation_mode_count, details.presentation_modes.data());
	}

	return details;
}

VkSampleCountFlagBits VulkanDevice::get_max_usable_sample_count()
{
	VkPhysicalDeviceProperties dev_prop{};
	vkGetPhysicalDeviceProperties(physical_device, &dev_prop);
	VkSampleCountFlags counts = std::min(dev_prop.limits.framebufferColorSampleCounts, dev_prop.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT)
		return VK_SAMPLE_COUNT_64_BIT;
	if (counts & VK_SAMPLE_COUNT_32_BIT)
		return VK_SAMPLE_COUNT_32_BIT;
	if (counts & VK_SAMPLE_COUNT_16_BIT)
		return VK_SAMPLE_COUNT_16_BIT;
	if (counts & VK_SAMPLE_COUNT_8_BIT)
		return VK_SAMPLE_COUNT_8_BIT;
	if (counts & VK_SAMPLE_COUNT_4_BIT)
		return VK_SAMPLE_COUNT_4_BIT;
	if (counts & VK_SAMPLE_COUNT_2_BIT)
		return VK_SAMPLE_COUNT_2_BIT;

	return VK_SAMPLE_COUNT_1_BIT;
}

VulkanDevice::VulkanDevice(VkInstance& inst, VkSurfaceKHR& srfc, bool enable_validation_layers, const std::vector<const char*>& validation_layers, VkQueue& graphics_queue, VkQueue& present_queue): instance(inst), surface(srfc)
{
	create_physical_device();
	create_device(enable_validation_layers, validation_layers, graphics_queue, present_queue);
}

VkFormatProperties VulkanDevice::get_format_properties(VkFormat& format)
{
	VkFormatProperties format_prop{};
	vkGetPhysicalDeviceFormatProperties(physical_device, format, &format_prop);
	return format_prop;
}

VkPhysicalDeviceMemoryProperties VulkanDevice::get_memory_properties()
{
	VkPhysicalDeviceMemoryProperties mem_props{};
	vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_props);
	return mem_props;
}
