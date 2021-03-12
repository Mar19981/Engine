#ifndef MODEL_H
#define MODEL_H
#define GLM_FORCE_RADIANS
#define GLM_FORCE_EXPERIMENTAL
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <array>
#include <memory>
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/hash.hpp"
#include "vulkan/vulkan.h"
#include <stb_image.h>
#include <tiny_obj_loader.h>
#include "VulkanDevice.h"


struct Vertex
{
	glm::vec3 pos;
	glm::vec3 colour;
	glm::vec2 tex_cord;
	static VkVertexInputBindingDescription get_binding_description()
	{
		VkVertexInputBindingDescription binding_description{};
		binding_description.binding = 0;
		binding_description.stride = sizeof(Vertex);
		binding_description.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return binding_description;
	}
	static std::array<VkVertexInputAttributeDescription, 3> get_attribute_descriptions()
	{
		std::array<VkVertexInputAttributeDescription, 3> attribute_descriptions{};
		attribute_descriptions.at(0).binding = attribute_descriptions.at(0).location = attribute_descriptions.at(1).binding = attribute_descriptions.at(2).binding = 0;
		attribute_descriptions.at(0).format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions.at(0).offset = offsetof(Vertex, pos);
		attribute_descriptions.at(1).location = 1;
		attribute_descriptions.at(1).format = VK_FORMAT_R32G32B32_SFLOAT;
		attribute_descriptions.at(1).offset = offsetof(Vertex, colour);
		attribute_descriptions.at(2).location = 2;
		attribute_descriptions.at(2).format = VK_FORMAT_R32G32_SFLOAT;
		attribute_descriptions.at(2).offset = offsetof(Vertex, tex_cord);

		return attribute_descriptions;
	}
	bool operator==(const Vertex& other) const {
		return pos == other.pos && colour == other.colour && tex_cord == other.tex_cord;
	}
};
namespace std 
{
	template<> struct hash<Vertex> {
		size_t operator()(Vertex const& vertex) const {
			return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.colour) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.tex_cord) << 1);
		}
	};
}
struct Uniform_buffer_object
{
	alignas(16) glm::mat4 model;
	alignas(16) glm::mat4 view;
	alignas(16) glm::mat4 proj;
};


class Model
{
	//Variables
	const std::string MODEL_PATH;
	std::string texture_path;
	glm::mat4 model_mat;
	uint32_t mip_levels;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indicies;
	int swap_chain_images_count;
	VkImage texture_img;
	VkImageView texture_img_view;
	VkDeviceMemory texture_mem;
	VkBuffer vertex_buffer;
	VkSampler texture_sampler;
	VkDeviceMemory vertex_memory;
	VkBuffer index_buffer;
	VkDeviceMemory index_mem;
	std::vector<VkBuffer>uniform_buffers;
	std::vector<VkDeviceMemory>uniform_mem;
	std::vector<VkDescriptorSet> descriptor_sets;
	//std::vector<VkCommandBuffer> command_buffers;
	glm::vec3 position;
	bool rotate_model = false;
	//Pointers
	std::shared_ptr<VulkanDevice> dev;
	VkDescriptorPool descriptor_pool;
	VkCommandPool command_pool;
	VkQueue graphics_queue;
	VkDescriptorSetLayout descriptor_set_layout;

	//Methods
	//void create_device()
	void create_texture_image();
	void create_texture_image_view();
	void create_texture_sampler();
	void load_model();
	void create_vertex_buffer();
	void create_index_buffer();
	void create_uniform_buffer();
	void create_descriptor_pool();
	void create_descriptor_sets();
	void create_buffer(VkDeviceSize size, VkBufferUsageFlags usage,
		VkMemoryPropertyFlags properties, VkBuffer& buffer,
		VkDeviceMemory& memory);
	void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags flags, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& mem, uint32_t mip_levels, VkSampleCountFlagBits num_samples);
	VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);
	VkCommandBuffer begin_single_time_commands();
	void end_single_time_commands(VkCommandBuffer command_buffer);
	void transition_image_layout(VkImage img, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
	void copy_buffer_to_img(VkBuffer buffer, VkImage img, uint32_t width, uint32_t height);
	void copy_buffer(VkBuffer src, VkBuffer dst, VkDeviceSize size);
	void generate_mipmaps(VkImage img, VkFormat format, int32_t width, int32_t height, uint32_t mip_levels);
	uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags prop);
	bool has_stencil_component(VkFormat format);
public:
	//Constructors and destructor
	Model(const std::string& model_path, const int swap_chain_images, VkCommandPool c_pool, 
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);	
	Model(const std::string& model_path, const std::string& tex_path, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Model(const std::string& model_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Model(const std::string& model_path, const std::string& tex_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	//Public methods
	void translate(const float x, const float y, const float z);
	void scale(const float amount);
	void scale(const float scale_x, const float scale_y, const float scale_z);
	void assign_texture(const std::string& tex_path);
	void rotate(const float x, const float y, const float z);
	void switch_animated_rotation();
	void init_model();
	glm::vec3 get_position() const;
	bool get_animation_state() const;
	VkDeviceMemory get_uniform_buffer_memory(uint32_t index) const;
	VkDeviceMemory get_vertex_buffer_memory() const;
	VkDeviceMemory get_index_buffer_memory() const;
	VkDeviceMemory get_texture_memory() const;
	VkImage get_texture_img() const;
	VkImageView get_texture_img_view() const;
	VkSampler get_texture_sampler() const;
	VkBuffer get_vertex_buffer() const;
	VkBuffer get_index_buffer() const;
	std::vector<VkBuffer> get_uniform_buffers() const;
	VkDescriptorPool get_descriptor_pool() const;
	std::vector<VkDescriptorSet> get_descriptor_sets() const;
	uint32_t get_indicies_size() const;
	void set_position(const float x, const float y, const float z);
	glm::mat4 get_model_matrix() const;
	void recreate_swap_chain_elements();
};
class Plane : public Model
{
public:
	Plane(const float width, const float height, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Plane(const float width, const float height, const std::string& tex_path, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Plane(const float width, const float height, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Plane(const float width, const float height, const std::string& tex_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
};
class Box : public Model
{
public:
	Box(const float width, const float height, const float depth, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Box(const float width, const float height, const float depth, const std::string& tex_path, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Box(const float width, const float height, const float depth, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Box(const float width, const float height, const float depth, const std::string& tex_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
};
class Sphere : public Model
{
public:
	Sphere(const float radious, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Sphere(const float radious, const std::string& tex_path, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Sphere(const float radious, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
	Sphere(const float radious, const std::string& tex_path, const float x, const float y, const float z, const int swap_chain_images, VkCommandPool c_pool,
		VkQueue g_queue, VkDescriptorSetLayout d_layout, std::shared_ptr<VulkanDevice> vd);
};
#endif