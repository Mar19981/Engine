#ifndef UTILITY_H
#define UTILITY_H
#include <utility>
#include "vulkan/vulkan.h"
struct Queue_family_indecies
{
	std::optional<uint32_t> graphics_family;
	std::optional<uint32_t> present_family;
	inline bool is_complete() { return graphics_family.has_value() && present_family.has_value(); };
};

struct Swap_chain_support_details
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentation_modes;
};



#endif // !UTILITY_H
