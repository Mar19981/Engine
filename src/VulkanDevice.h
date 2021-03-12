#ifndef VULKANDEVICE_H
#define VULKANDEVICE_H
#include <optional>
#include <vector>
#include <set>
#include <iostream>
#include <algorithm>
#include "vulkan/vulkan.h"
#include "utility.h"
	class VulkanDevice
	{
		VkInstance instance;
		VkSurfaceKHR& surface;
		VkPhysicalDevice physical_device = VK_NULL_HANDLE;
		VkDevice device;
		VkSampleCountFlagBits msaa_samples = VK_SAMPLE_COUNT_1_BIT;
		uint32_t supported_extension_count;
		const std::vector<const char*>device_extensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
		std::vector<VkExtensionProperties> supported_extensions;
		void create_physical_device();
		bool check_device_extension_support(const VkPhysicalDevice& dev);
		bool is_device_suitable(const VkPhysicalDevice& dev);
		void create_device(bool enable_validation_layers, const std::vector<const char*>& validation_layers, VkQueue& graphics_queue, VkQueue& present_queue);
		VkSampleCountFlagBits get_max_usable_sample_count();
	public:
		VulkanDevice(VkInstance& inst, VkSurfaceKHR& srfc, bool enable_validation_layers, const std::vector<const char*>& validation_layers, VkQueue& graphics_queue, VkQueue& present_queue);
		VkFormatProperties get_format_properties(VkFormat& format);
		VkPhysicalDeviceMemoryProperties get_memory_properties();
		Queue_family_indecies find_queue_family_indicies(const VkPhysicalDevice& dev);
		Swap_chain_support_details query_swap_chain_support(const VkPhysicalDevice& dev);
		inline VkSampleCountFlagBits get_msaa_samples() { return msaa_samples; };
		inline VkDevice& get_device() { return device; };
		inline VkPhysicalDevice get_physical_device() { return physical_device; };
	};
#endif

