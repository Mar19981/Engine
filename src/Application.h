#ifndef APPLICATION_H
#define APPLICATION_H
#define GLM_FORCE_RADIANS
#define GLM_FORCE_EXPERIMENTAL
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <cstring>
#include <utility>
#include <memory>
#include <set>
#include <unordered_map>
#include <map>
#include <array>
#include <exception>
#include <optional>
#include <limits>
#include <chrono>
#include "vulkan/vulkan.h"
#include "GLFW/glfw3.h"
#include "glm/glm.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtx/hash.hpp"
#include "camera.h"
#include "model.h"
#include "shader.h"
#include "VulkanDevice.h"
#include "utility.h"
#ifdef RELEASE
const bool enable_validation_layers = false;
#else
const bool enable_validation_layers = true;
#endif
	VkResult create_debug_utils_messenger_EXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* create_info_pointer,
		const VkAllocationCallbacks* allocator_pointer,
		VkDebugUtilsMessengerEXT* messenger_pointer
	);
	void destroy_debug_utils_messenger_EXT(VkInstance instance,
		VkDebugUtilsMessengerEXT messenger,
		const VkAllocationCallbacks* allocator_pointer
	);

	class Engine
	{
	public:
		Engine(const std::string& name, const int width, const int height);
		~Engine();
		void run();

		void toogle_wireframe();
		//Camera functions
		void create_camera();
		void create_camera(const float x, const float y, const float z);
		void translate_camera(const int id, const float x, const float y, const float z);
		void set_camera_position(const int id, const float x, const float y, const float z);
		void rotate_camera(const int id, const float yaw, const float pitch, const float roll);
		void change_fov(const int id, const float fov);
		//Model functions
		void create_model(const std::string& model_path);
		void create_model(const std::string& model_path, const float x, const float y, const float z);
		void create_model(const std::string& model_path, const std::string& tex_path);
		void create_model(const std::string& model_path, const std::string& tex_path, const float x, const float y, const float z);
		void create_sphere(const float radious);
		void create_sphere(const float radious, const float x, const float y, const float z);
		void create_sphere(const float radious, const std::string& tex_path, const float x, const float y, const float z);
		void create_sphere(const float radious, const std::string& tex_path);
		void create_plane(const float width, const float height);
		void create_plane(const float width, const float height, const float x, const float y, const float z);
		void create_plane(const float width, const float height, const std::string& tex_path);
		void create_plane(const float width, const float height, const std::string& tex_path, const float x, const float y, const float z);
		void create_box(const float width, const float height, const float length);
		void create_box(const float width, const float height, const float length, const float x, const float y, const float z);
		void create_box(const float width, const float height, const float length, const std::string& tex_path);
		void create_box(const float width, const float height, const float length, const std::string& tex_path, const float x, const float y, const float z);
		void translate_model(const int id, const float x, const float y, const float z);
		void rotate_model(const int id, const float x, const float y, const float z);
		void scale_model(const int id, const float x, const float y, const float z);
		void change_texture(const int id, const std::string& path);
		void switch_animated_rotation(const int id);


	private:
		//User defined attributes
		std::string app_name;
		const int WIDTH, HEIGHT;
		int current_frame = 0;
		const int MAX_FRAMES_IN_FLIGHT = 2;
		std::vector<Free_camera> cameras;
		Free_camera* active_camera;
		int camera_index;
		std::vector<std::unique_ptr<Model>> models;
		uint32_t aspect_ratio;
		static float delta_time;
		static float last_frame;
		VkInstance instance;
		VkSurfaceKHR surface;
		VkDebugUtilsMessengerEXT messenger;
		std::shared_ptr<VulkanDevice> vulkan_device;
		VkQueue graphics_queue;
		VkQueue present_queue;
		VkSwapchainKHR swap_chain;
		std::vector<VkImage> swap_chain_images;
		VkFormat swap_chain_image_format;
		VkExtent2D swap_chain_extent;
		std::vector<VkImageView>swap_chain_img_views;
		VkRenderPass render_pass;
		VkDescriptorSetLayout descriptor_set_layout;
		VkPipelineLayout pipeline_layout;
		VkPipeline pipeline;
		VkImage depth_img;
		VkDeviceMemory depth_mem;
		VkImageView depth_img_view;
		VkImage colour_img;
		VkDeviceMemory colour_mem;
		VkImageView colour_img_view;
		std::vector<VkFramebuffer> swap_chain_framebuffers;
		VkCommandPool command_pool;
		std::vector<VkCommandBuffer> command_buffers;
		std::vector<VkSemaphore> image_available_semaphores, rendering_finished_semaphores;
		std::vector<VkFence> in_flight_fences, images_in_flight;
		bool framebuffer_resized = false;
		GLFWwindow* window;
		std::vector<VkExtensionProperties> supported_extensions;
		uint32_t supported_extension_count;
		const std::vector<const char*> validation_layers = { "VK_LAYER_KHRONOS_validation" };
		std::pair<VkPolygonMode, std::string> poly_mode = std::make_pair(VK_POLYGON_MODE_FILL, R"(src\frag.spv)");
		std::vector<const char*> get_required_ext();
		void create_instance();
		bool check_valid_layer_supp();
		void debug_messenger_setup();
		void create_swap_chain();
		void create_image_views();
		void create_descriptor_set_layout();
		void create_graphics_pipeline();
		void create_render_passes();
		void create_framebuffers();
		void create_command_pool();
		VkImageView create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels);
		void create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling,
			VkImageUsageFlags flags, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& mem, uint32_t mip_levels, VkSampleCountFlagBits num_samples);
		VkCommandBuffer begin_single_time_commands();
		void end_single_time_commands(VkCommandBuffer command_buffer);
		void transition_image_layout(VkImage img, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels);
		void create_command_buffers();
		void create_semaphores_and_fences();
		void create_colour_resources();
		void create_depth_resources();
		void clean_swap_chain();
		void update_uniform_buffer(uint32_t index);
		void draw_frame();
		void recreate_swap_chain();
		void process_input();
		void info();
		VkSurfaceFormatKHR choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats);
		VkPresentModeKHR choose_presentation_mode(const std::vector <VkPresentModeKHR>& modes);
		VkExtent2D choose_swap_extend(const VkSurfaceCapabilitiesKHR& cap);
		uint32_t find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags prop);
		VkFormat find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags flags);
		VkFormat find_depth_format();
		bool has_stencil_component(VkFormat format);
		//Static functions
		static VKAPI_ATTR VkBool32 VKAPI_CALL debug_callback(
			VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
			VkDebugUtilsMessageTypeFlagsEXT messageType,
			const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
			void* pUserData);
		static void framebuffer_resize(GLFWwindow* window, int width, int height);
		static void camera_switch_callback(GLFWwindow* window, int key, int scancode, int action, int mods);
	};

#endif // !
