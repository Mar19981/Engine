#include "shader.h"

Shader::Shader(const std::string& path, VkDevice& dev): device(dev)
{
	load_shader(path);
}

[[ noreturn ]] void Shader::load_shader(const std::string& path)
{
	std::ifstream input(path, std::ios_base::binary | std::ios_base::ate);
	if (!input)
		throw std::runtime_error("Failed to open a file '" + path + "'!\n");
	else
	{
		code.clear();
		int file_size = static_cast<int>(input.tellg());
		code.resize(file_size);
		input.seekg(std::ios_base::beg);
		input.read(code.data(), file_size);
		input.close();
	}

}

VkShaderModule Shader::create_shader_module()
{
	VkShaderModuleCreateInfo info{};
	info.codeSize = code.size();
	info.pCode = reinterpret_cast<const uint32_t*>(code.data());
	info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	VkShaderModule shader_module;
	if (vkCreateShaderModule(device, &info, nullptr, &shader_module) != VK_SUCCESS)
		throw std::runtime_error("Failed to create shader module!\n");
	return shader_module;
}
