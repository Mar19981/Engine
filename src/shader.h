#pragma once
#ifndef SHADER_H
#define SHADER_H
#include <string>
#include <vector>
#include <fstream>
#include "vulkan/vulkan.h"
class Shader
{
	std::vector<char> code;
	VkDevice& device;
public:
	Shader(const std::string& path, VkDevice& dev);
	void load_shader(const std::string& path);
	VkShaderModule create_shader_module();
};
#endif // !SHADER_H
