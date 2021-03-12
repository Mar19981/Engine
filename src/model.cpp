#define STB_IMAGE_IMPLEMENTATION
#define TINYOBJLOADER_IMPLEMENTATION
#include "model.h"
void Model::translate(const float x, const float y, const float z)
{
	model_mat = glm::translate(model_mat, glm::vec3(x, y, z));
}

void Model::scale(const float amount)
{
	if (amount != 0.0f)
		model_mat = glm::scale(model_mat, glm::vec3(amount));
}

void Model::scale(const float scale_x, const float scale_y, const float scale_z)
{
	if (scale_x != 0.0f && scale_y != 0.0f && scale_z != 0.0f)
		model_mat = glm::scale(model_mat, glm::vec3(scale_x, scale_y, scale_z));
}

void Model::assign_texture(const std::string& tex_path)
{
	texture_path = tex_path;
	create_descriptor_pool();
	create_texture_image();
	create_texture_image_view();
	create_texture_sampler();
	create_descriptor_sets();
}

void Model::rotate(const float x, const float y, const float z)
{
	model_mat = glm::rotate(model_mat, glm::radians(x), glm::vec3(1.0f, 0.0f, 0.0f));
	model_mat = glm::rotate(model_mat, glm::radians(y), glm::vec3(0.0f, 1.0f, 0.0f));
	model_mat = glm::rotate(model_mat, glm::radians(y), glm::vec3(0.0f, 0.0f, 1.0f));
}

void Model::switch_animated_rotation()
{
	rotate_model = !rotate_model;
}

glm::vec3 Model::get_position() const
{
	return position;
}

bool Model::get_animation_state() const
{
	return rotate_model;
}

VkDeviceMemory Model::get_uniform_buffer_memory(uint32_t index) const
{
	return uniform_mem.at(index);
}

VkDeviceMemory Model::get_vertex_buffer_memory() const
{
	return vertex_memory;
}

VkDeviceMemory Model::get_index_buffer_memory() const
{
	return index_mem;
}

VkDeviceMemory Model::get_texture_memory() const
{
	return texture_mem;
}

VkImage Model::get_texture_img() const
{
	return texture_img;
}

VkImageView Model::get_texture_img_view() const
{
	return texture_img_view;
}

VkSampler Model::get_texture_sampler() const
{
	return texture_sampler;
}

VkBuffer Model::get_vertex_buffer() const
{
	return vertex_buffer;
}

VkBuffer Model::get_index_buffer() const
{
	return index_buffer;
}

std::vector<VkBuffer> Model::get_uniform_buffers() const
{
	return uniform_buffers;
}

VkDescriptorPool Model::get_descriptor_pool() const
{
	return descriptor_pool;
}

std::vector<VkDescriptorSet> Model::get_descriptor_sets() const
{
	return descriptor_sets;
}

uint32_t Model::get_indicies_size() const
{
	return static_cast<uint32_t>(indicies.size());
}

void Model::set_position(const float x, const float y, const float z)
{
	position = glm::vec3(x, y, z);
}

glm::mat4 Model::get_model_matrix() const
{
	return model_mat;
}

void Model::recreate_swap_chain_elements()
{
	create_descriptor_pool();
	create_uniform_buffer();
	create_descriptor_sets();
}

void Model::init_model()
{
	create_descriptor_pool();
	create_texture_image();
	create_texture_image_view();
	create_texture_sampler();
	load_model();
	create_vertex_buffer();
	create_index_buffer();
	create_uniform_buffer();
	create_descriptor_sets();
}

void Model::create_texture_image()
{
	int tex_width, tex_height, tex_channels;
	stbi_uc* pixels = stbi_load(texture_path.c_str(), &tex_width, &tex_height, &tex_channels, STBI_rgb_alpha);
	VkDeviceSize img_size = tex_width * tex_height * 4;
	if (!pixels)
		throw std::runtime_error("Failed to load texture file!\n");
	mip_levels = static_cast<uint32_t>(std::floor(std::log2(std::max(tex_width, tex_height)))) + 1;
	VkBuffer staging_buffer{};
	VkDeviceMemory staging_memory{};
	create_buffer(img_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer, staging_memory);
	void* data;
	vkMapMemory(dev->get_device(), staging_memory, 0, img_size, 0, &data);
	memcpy(data, pixels, static_cast<size_t>(img_size));
	vkUnmapMemory(dev->get_device(), staging_memory);
	stbi_image_free(pixels);
	create_image(tex_width, tex_height, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL,
		VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		texture_img, texture_mem, mip_levels, VK_SAMPLE_COUNT_1_BIT);
	transition_image_layout(texture_img, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mip_levels);
	copy_buffer_to_img(staging_buffer, texture_img, static_cast<uint32_t>(tex_width), static_cast<uint32_t>(tex_height));
	generate_mipmaps(texture_img, VK_FORMAT_R8G8B8A8_UNORM, tex_width, tex_height, mip_levels);
	vkDestroyBuffer(dev->get_device(), staging_buffer, nullptr);
	vkFreeMemory(dev->get_device(), staging_memory, nullptr);
}

void Model::create_texture_image_view()
{
	texture_img_view = create_image_view(texture_img, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, mip_levels);
}

void Model::create_texture_sampler()
{

	VkSamplerCreateInfo sampler_info{};
	sampler_info.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler_info.magFilter = sampler_info.minFilter = VK_FILTER_LINEAR;
	sampler_info.addressModeU = sampler_info.addressModeV = sampler_info.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	sampler_info.anisotropyEnable = VK_TRUE;
	sampler_info.maxAnisotropy = 16;
	sampler_info.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	sampler_info.unnormalizedCoordinates = sampler_info.compareEnable = VK_FALSE;
	sampler_info.compareOp = VK_COMPARE_OP_ALWAYS;
	sampler_info.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler_info.maxLod = static_cast<float>(mip_levels);
	sampler_info.minLod = sampler_info.mipLodBias = 0.0f;
	if (vkCreateSampler(dev->get_device(), &sampler_info, nullptr, &texture_sampler) != VK_SUCCESS)
		throw std::runtime_error("Failed to create image sampler!\n");
}

void Model::load_model()
{
	tinyobj::attrib_t attrib{};
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;
	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str()))
		throw std::runtime_error(warn + err);
	std::unordered_map<Vertex, uint32_t>unique_vertices{};
	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos = {
				attrib.vertices[3 * index.vertex_index + 0],
				attrib.vertices[3 * index.vertex_index + 1],
				attrib.vertices[3 * index.vertex_index + 2]
			};
			vertex.tex_cord = {
				attrib.texcoords[2 * index.texcoord_index + 0],
				1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
			};
			vertex.colour = { 1.0f, 1.0f, 1.0f };
			if (unique_vertices.count(vertex) == 0)
			{
				unique_vertices[vertex] = static_cast<uint32_t>(vertices.size());
				vertices.push_back(vertex);
			}
			indicies.push_back(unique_vertices[vertex]);
		}
	}
}

void Model::create_vertex_buffer()
{

	VkDeviceSize buffer_size = sizeof(vertices.at(0)) * vertices.size();
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer, staging_memory);
	void* data;
	vkMapMemory(dev->get_device(), staging_memory, 0, buffer_size, 0, &data);
	memcpy(data, vertices.data(), static_cast<size_t>(buffer_size));
	vkUnmapMemory(dev->get_device(), staging_memory);
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertex_buffer, vertex_memory);
	copy_buffer(staging_buffer, vertex_buffer, buffer_size);
	vkDestroyBuffer(dev->get_device(), staging_buffer, nullptr);
	vkFreeMemory(dev->get_device(), staging_memory, nullptr);
}

void Model::create_index_buffer()
{
	VkDeviceSize buffer_size = sizeof(indicies.at(0)) * indicies.size();
	VkBuffer staging_buffer;
	VkDeviceMemory staging_memory;
	create_buffer(buffer_size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		staging_buffer, staging_memory);
	void* data{};
	vkMapMemory(dev->get_device(), staging_memory, 0, buffer_size, 0, &data);
	memcpy(data, indicies.data(), static_cast<size_t>(buffer_size));
	vkUnmapMemory(dev->get_device(), staging_memory);
	create_buffer(buffer_size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, index_buffer, index_mem);
	copy_buffer(staging_buffer, index_buffer, buffer_size);
	vkDestroyBuffer(dev->get_device(), staging_buffer, nullptr);
	vkFreeMemory(dev->get_device(), staging_memory, nullptr);
}

void Model::create_uniform_buffer()
{
	VkDeviceSize buffer_size = sizeof(Uniform_buffer_object);
	uniform_buffers.resize(swap_chain_images_count);
	uniform_mem.resize(swap_chain_images_count);
	for (int i = 0; i < swap_chain_images_count; ++i)
	{
		create_buffer(buffer_size, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			uniform_buffers.at(i), uniform_mem.at(i));
	}
}

void Model::create_descriptor_pool()
{
	std::array<VkDescriptorPoolSize, 2> pool_sizes{};
	pool_sizes.at(0).descriptorCount = pool_sizes.at(1).descriptorCount = static_cast<uint32_t>(swap_chain_images_count);
	pool_sizes.at(0).type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes.at(1).type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	VkDescriptorPoolCreateInfo pool_info{};
	pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	pool_info.poolSizeCount = static_cast<uint32_t>(pool_sizes.size());
	pool_info.pPoolSizes = pool_sizes.data();
	pool_info.maxSets = static_cast<uint32_t>(swap_chain_images_count);
	if (vkCreateDescriptorPool(dev->get_device(), &pool_info, nullptr, &descriptor_pool) != VK_SUCCESS)
		throw std::runtime_error("Failed to create descriptor pool!\n");

}


void Model::create_descriptor_sets()
{
	std::vector<VkDescriptorSetLayout> layouts(swap_chain_images_count, descriptor_set_layout);
	VkDescriptorSetAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	alloc_info.descriptorPool = descriptor_pool;
	alloc_info.descriptorSetCount = static_cast<uint32_t>(swap_chain_images_count);
	alloc_info.pSetLayouts = layouts.data();

	descriptor_sets.resize(swap_chain_images_count);
	if (vkAllocateDescriptorSets(dev->get_device(), &alloc_info, descriptor_sets.data()) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate descriptor sets!\n");
	for (int i = 0; i < swap_chain_images_count; ++i)
	{
		VkDescriptorBufferInfo buffer_info{};
		buffer_info.offset = 0;
		buffer_info.buffer = uniform_buffers.at(i);
		buffer_info.range = sizeof(Uniform_buffer_object);
		VkDescriptorImageInfo img_info{};
		img_info.sampler = texture_sampler;
		img_info.imageView = texture_img_view;
		img_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		std::array<VkWriteDescriptorSet, 2> descriptor_writes{};
		descriptor_writes.at(0).sType = descriptor_writes.at(1).sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptor_writes.at(0).descriptorCount = descriptor_writes.at(1).descriptorCount = 1;
		descriptor_writes.at(0).dstSet = descriptor_writes.at(1).dstSet = descriptor_sets.at(i);
		descriptor_writes.at(0).dstArrayElement = descriptor_writes.at(1).dstArrayElement = 0;
		descriptor_writes.at(0).dstBinding = 0;
		descriptor_writes.at(1).dstBinding = 1;
		descriptor_writes.at(0).descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptor_writes.at(1).descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptor_writes.at(0).pBufferInfo = &buffer_info;
		descriptor_writes.at(1).pImageInfo = &img_info;

		vkUpdateDescriptorSets(dev->get_device(), static_cast<uint32_t>(descriptor_writes.size()), descriptor_writes.data(), 0, nullptr);
	}
}
void Model::create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags flags, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& mem, uint32_t mip_levels, VkSampleCountFlagBits num_samples)
{
	VkImageCreateInfo img_info{};
	img_info.mipLevels = mip_levels;
	img_info.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	img_info.imageType = VK_IMAGE_TYPE_2D;
	img_info.extent.width = width;
	img_info.extent.height = height;
	img_info.extent.depth = img_info.arrayLayers = 1;
	img_info.format = format;
	img_info.tiling = tiling;
	img_info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	img_info.usage = flags;
	img_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	img_info.samples = num_samples;
	if (vkCreateImage(dev->get_device(), &img_info, nullptr, &img) != VK_SUCCESS)
		throw std::runtime_error("Failed to create texture image!\n");
	VkMemoryRequirements mem_req{};
	vkGetImageMemoryRequirements(dev->get_device(), img, &mem_req);
	VkMemoryAllocateInfo malloc_info{};
	malloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	malloc_info.allocationSize = mem_req.size;
	malloc_info.memoryTypeIndex = find_memory_type(mem_req.memoryTypeBits, properties);
	if (vkAllocateMemory(dev->get_device(), &malloc_info, nullptr, &mem) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate texture memory!\n");
	vkBindImageMemory(dev->get_device(), img, mem, 0);
}

VkImageView Model::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels)
{
	VkImageViewCreateInfo info{};
	info.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	info.format = format;
	info.image = image;
	info.viewType = VK_IMAGE_VIEW_TYPE_2D;
	info.subresourceRange.aspectMask = aspect_flags;
	info.subresourceRange.baseMipLevel = info.subresourceRange.baseArrayLayer = 0;
	info.subresourceRange.layerCount = 1;
	info.subresourceRange.levelCount = mip_levels;
	VkImageView img_view;
	if (vkCreateImageView(dev->get_device(), &info, nullptr, &img_view) != VK_SUCCESS)
		throw std::runtime_error("Failed to create texture image view!\n");
	return img_view;
}

VkCommandBuffer Model::begin_single_time_commands()
{
	VkCommandBufferAllocateInfo alloc_info{};
	alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	alloc_info.commandBufferCount = 1;
	alloc_info.commandPool = command_pool;
	alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

	VkCommandBuffer command_buffer;
	vkAllocateCommandBuffers(dev->get_device(), &alloc_info, &command_buffer);
	VkCommandBufferBeginInfo begin_info{};
	begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
	vkBeginCommandBuffer(command_buffer, &begin_info);

	return command_buffer;
}

void Model::end_single_time_commands(VkCommandBuffer command_buffer)
{
	vkEndCommandBuffer(command_buffer);
	VkSubmitInfo submit_info{};
	submit_info.commandBufferCount = 1;
	submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submit_info.pCommandBuffers = &command_buffer;
	vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
	vkDeviceWaitIdle(dev->get_device());
	vkFreeCommandBuffers(dev->get_device(), command_pool, 1, &command_buffer);
}

void Model::transition_image_layout(VkImage img, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels)
{
	auto command_buffer = begin_single_time_commands();

	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = img;
	barrier.oldLayout = old_layout;
	barrier.newLayout = new_layout;
	barrier.srcQueueFamilyIndex = barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	if (new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (has_stencil_component(format))
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
	}
	else
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = mip_levels;
	barrier.subresourceRange.baseArrayLayer = barrier.subresourceRange.baseMipLevel = 0;
	VkPipelineStageFlags source_stage{}, destination_stage{};
	if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && new_layout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		source_stage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destination_stage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT;
		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else if (old_layout == VK_IMAGE_LAYOUT_UNDEFINED && new_layout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		source_stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destination_stage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	else
		throw std::runtime_error("Unsupported layout transition!\n");

	vkCmdPipelineBarrier(command_buffer, source_stage, destination_stage, 0, 0, nullptr, 0, nullptr, 1, &barrier);


	end_single_time_commands(command_buffer);
}

void Model::create_buffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& memory)
{
	VkBufferCreateInfo buffer_info{};
	buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	buffer_info.usage = usage;
	buffer_info.size = size;
	buffer_info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	if (vkCreateBuffer(dev->get_device(), &buffer_info, nullptr, &buffer) != VK_SUCCESS)
		throw std::runtime_error("Failed to create vertex buffer!\n");
	VkMemoryRequirements mem_req{};
	vkGetBufferMemoryRequirements(dev->get_device(), buffer, &mem_req);
	VkMemoryAllocateInfo malloc_info{};
	malloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	malloc_info.allocationSize = mem_req.size;
	malloc_info.memoryTypeIndex = find_memory_type(mem_req.memoryTypeBits, properties);
	if (vkAllocateMemory(dev->get_device(), &malloc_info, nullptr, &memory) != VK_SUCCESS)
		throw std::runtime_error("Failed to allocate memory for the vertex buffer!\n");
	vkBindBufferMemory(dev->get_device(), buffer, memory, 0);
}

void Model::copy_buffer_to_img(VkBuffer buffer, VkImage img, uint32_t width, uint32_t height)
{
	auto command_buffer = begin_single_time_commands();

	VkBufferImageCopy img_cpy{};
	img_cpy.bufferOffset = img_cpy.bufferRowLength = img_cpy.bufferImageHeight = 0;
	img_cpy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	img_cpy.imageSubresource.layerCount = 1;
	img_cpy.imageSubresource.baseArrayLayer = img_cpy.imageSubresource.mipLevel = 0;
	img_cpy.imageOffset = { 0, 0, 0 };
	img_cpy.imageExtent = { width, height, 1 };
	vkCmdCopyBufferToImage(command_buffer, buffer, img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &img_cpy);
	end_single_time_commands(command_buffer);
}

void Model::copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size)
{
	auto command_buffer = begin_single_time_commands();
	VkBufferCopy buffer_region{};
	buffer_region.size = size;
	buffer_region.srcOffset = buffer_region.dstOffset = 0;
	vkCmdCopyBuffer(command_buffer, src, dst, 1, &buffer_region);
	end_single_time_commands(command_buffer);
}

void Model::generate_mipmaps(VkImage img, VkFormat format, int32_t width, int32_t height, uint32_t mip_levels)
{
	if (!(dev->get_format_properties(format).optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT))
		throw std::runtime_error("Texture image format doesn't support linear blitting!\n");
	VkCommandBuffer command_buffer = begin_single_time_commands();
	VkImageMemoryBarrier barrier{};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = img;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = barrier.subresourceRange.levelCount = 1;
	barrier.dstQueueFamilyIndex = barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	int32_t mip_width = width, mip_height = height;
	for (uint32_t i = 1; i < mip_levels; ++i)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		vkCmdPipelineBarrier(
			command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT,
			0, 0, nullptr, 0, nullptr, 1, &barrier);
		VkImageBlit blit{};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mip_width, mip_height, 1 };
		blit.srcSubresource.aspectMask = blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.baseArrayLayer = blit.dstSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = blit.dstSubresource.layerCount = 1;
		blit.srcSubresource.mipLevel = i - 1;
		blit.dstOffsets[1] = { mip_width > 1 ? mip_width / 2 : 1,
			mip_height > 1 ? mip_height / 2 : 1, 1 };
		blit.dstSubresource.mipLevel = i;
		vkCmdBlitImage(command_buffer, img, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			img, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, VK_FILTER_LINEAR);
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
		vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
			VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);
		if (mip_width > 1) { mip_width /= 2; }
		if (mip_height > 2) { mip_height /= 2; }
	}
	barrier.subresourceRange.baseMipLevel = mip_levels - 1;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	vkCmdPipelineBarrier(command_buffer, VK_PIPELINE_STAGE_TRANSFER_BIT,
		VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrier);

	end_single_time_commands(command_buffer);
}

uint32_t Model::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags prop)
{
	VkPhysicalDeviceMemoryProperties mem_prop = dev->get_memory_properties();
	//vkGetPhysicalDeviceMemoryProperties(physical_device, &mem_prop);
	for (uint32_t i = 0; i < mem_prop.memoryTypeCount; ++i)
	{
		if (type_filter & (1 << i) &&
			((mem_prop.memoryTypes[i].propertyFlags & prop) == prop))
			return i;
	}
	throw std::runtime_error("Failed to find suitable memory type!\n");
}

bool Model::has_stencil_component(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

Model::Model(const std::string& model_path, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) :
	MODEL_PATH(model_path), swap_chain_images_count(swap_chain_images), command_pool(c_pool), graphics_queue(g_queue), descriptor_set_layout(d_layout), dev(vd)
{
	texture_path = R"(src\tex\checker.jpg)";
	position = glm::vec3(0.0f);
	model_mat = glm::mat4(1.0f);
}

Model::Model(const std::string& model_path, const std::string& tex_path, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(model_path, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	texture_path = tex_path;
}

Model::Model(const std::string& model_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(model_path, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	position = glm::vec3(x, y, z);
	model_mat = glm::translate(model_mat, position);
}

Model::Model(const std::string& model_path, const std::string& tex_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(model_path, tex_path, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	position = glm::vec3(x, y, z);
	model_mat = glm::translate(model_mat, position);
}

Plane::Plane(const float width, const float height, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\plane.obj)", swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(width, 1.0, height);
}

Plane::Plane(const float width, const float height, const std::string& tex_path, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\plane.obj)", tex_path, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(width, 1.0, height);
}

Plane::Plane(const float width, const float height, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\plane.obj)", x, y, z, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(width, 1.0, height);
}

Plane::Plane(const float width, const float height, const std::string& tex_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\plane.obj)", tex_path, x, y, z, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(width, 1.0, height);
}
Box::Box(const float width, const float height, const float depth, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\box.obj)", swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(width, height, depth);
}

Box::Box(const float width, const float height, const float depth, const std::string& tex_path, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\box.obj)", tex_path, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(width, height, depth);
}

Box::Box(const float width, const float height, const float depth, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\box.obj)", x, y, z, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(width, height, depth);
}

Box::Box(const float width, const float height, const float depth, const std::string& tex_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\box.obj)", tex_path, x, y, z, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(width, height, depth);
}
Sphere::Sphere(const float radious, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\sphere.obj)", swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(radious);
}

Sphere::Sphere(const float radious, const std::string& tex_path, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\sphere.obj)", tex_path, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(radious);
}

Sphere::Sphere(const float radious, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\sphere.obj)", x, y, z, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(radious);
}

Sphere::Sphere(const float radious, const std::string& tex_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool, VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd) : Model(R"(src\models\sphere.obj)", tex_path, x, y, z, swap_chain_images, c_pool, g_queue, d_layout, vd)
{
	scale(radious);
}

