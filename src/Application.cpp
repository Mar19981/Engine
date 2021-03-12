#include "Application.h"

float Engine::delta_time = 0.0f;
float Engine::last_frame = 0.0f;


	VkResult create_debug_utils_messenger_EXT(VkInstance instance,
		const VkDebugUtilsMessengerCreateInfoEXT* create_info_pointer,
		const VkAllocationCallbacks* allocator_pointer,
		VkDebugUtilsMessengerEXT* messenger_pointer
	)
	{
		auto function = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
		if (function)
			return function(instance, create_info_pointer, allocator_pointer, messenger_pointer);
		else
			return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
	void destroy_debug_utils_messenger_EXT(VkInstance instance,
		VkDebugUtilsMessengerEXT messenger,
		const VkAllocationCallbacks* allocator_pointer
	)
	{
		auto function = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
		if (function)
			function(instance, messenger, nullptr);
	}


	Engine::Engine(const std::string& name, const int width, const int height) : app_name(name), WIDTH(width), HEIGHT(height), camera_index(0)
	{
		//GLFW init
		glfwInit();
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		window = glfwCreateWindow(WIDTH, HEIGHT, app_name.c_str(), nullptr, nullptr);
		glfwSetWindowUserPointer(window, this);
		glfwSetFramebufferSizeCallback(window, framebuffer_resize);
		glfwSetKeyCallback(window, camera_switch_callback);
		//Vulkan init
		if (enable_validation_layers && !check_valid_layer_supp())
			throw std::runtime_error("Validation layers are not supported!");

		create_instance();
		debug_messenger_setup();
		if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS)
			throw std::runtime_error("Unable to create window surface!\n");
		//create_physical_device();
		//create_device();
		vulkan_device = std::make_shared<VulkanDevice>(instance, surface, enable_validation_layers, validation_layers, graphics_queue, present_queue);
		create_swap_chain();
		cameras.emplace_back(Free_camera(aspect_ratio));
		active_camera = &cameras.front();
		create_image_views();
		create_render_passes();
		create_descriptor_set_layout();
		create_graphics_pipeline();
		create_command_pool();
		create_colour_resources();
		create_depth_resources();
		create_framebuffers();

		models.emplace_back(std::make_unique<Model>(R"(src\models\teapot.obj)", R"(src\tex\tex1.jpg)", 0.4f, 1.0f, -0.3f, swap_chain_images.size(), command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.at(0)->init_model();
		models.at(0)->scale(0.5f);
		models.at(0)->switch_animated_rotation();
		models.emplace_back(std::make_unique<Model>(R"(src\models\sphere.obj)", 2.0f, 2.0f, 0.0f, swap_chain_images.size(), command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.at(1)->init_model();
		create_semaphores_and_fences();

	}
	Engine::~Engine()
	{
		if (enable_validation_layers)
			destroy_debug_utils_messenger_EXT(instance, messenger, nullptr);
		clean_swap_chain();
		for (int i = 0; i < models.size(); ++i)
		{
		vkDestroySampler(vulkan_device->get_device(), models.at(i)->get_texture_sampler(), nullptr);
		vkDestroyImageView(vulkan_device->get_device(), models.at(i)->get_texture_img_view(), nullptr);
		vkDestroyImage(vulkan_device->get_device(), models.at(i)->get_texture_img(), nullptr);
		vkFreeMemory(vulkan_device->get_device(), models.at(i)->get_texture_memory(), nullptr);
		vkDestroyBuffer(vulkan_device->get_device(), models.at(i)->get_index_buffer(), nullptr);
		vkFreeMemory(vulkan_device->get_device(), models.at(i)->get_index_buffer_memory(), nullptr);
		vkDestroyBuffer(vulkan_device->get_device(), models.at(i)->get_vertex_buffer(), nullptr);
		vkFreeMemory(vulkan_device->get_device(), models.at(i)->get_vertex_buffer_memory(), nullptr);
		}
		vkDestroyDescriptorSetLayout(vulkan_device->get_device(), descriptor_set_layout, nullptr);
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
		{
			vkDestroySemaphore(vulkan_device->get_device(), image_available_semaphores.at(i), nullptr);
			vkDestroySemaphore(vulkan_device->get_device(), rendering_finished_semaphores.at(i), nullptr);
			vkDestroyFence(vulkan_device->get_device(), in_flight_fences.at(i), nullptr);
		}
		vkDestroyCommandPool(vulkan_device->get_device(), command_pool, nullptr);
		vkDestroyDevice(vulkan_device->get_device(), nullptr);
		vkDestroySurfaceKHR(instance, surface, nullptr);
		vkDestroyInstance(instance, nullptr);
		glfwDestroyWindow(window);
		glfwTerminate();
	}
	void Engine::run()
	{
		create_command_buffers();
		info();
		while (!glfwWindowShouldClose(window))
		{
			float current_frame = glfwGetTime();
			delta_time = current_frame - last_frame;
			last_frame = current_frame;
			glfwPollEvents();
			process_input();
			draw_frame();		
		
		}
		vkDeviceWaitIdle(vulkan_device->get_device());
	}
	void Engine::toogle_wireframe()
	{
		if (poly_mode.first == VK_POLYGON_MODE_FILL)
		{
			poly_mode.first = VK_POLYGON_MODE_LINE;
			poly_mode.second = R"(src\frag_wire.spv)";
		}
		else
		{
			poly_mode.first = VK_POLYGON_MODE_FILL;
			poly_mode.second = R"(src\frag.spv)";
		}
		recreate_swap_chain();

	}
	void Engine::change_texture(const int id, const std::string& path)
	{
		vkFreeCommandBuffers(vulkan_device->get_device(), command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
		vkDestroyDescriptorPool(vulkan_device->get_device(), models.at(id)->get_descriptor_pool(), nullptr);
		vkDestroySampler(vulkan_device->get_device(), models.at(id)->get_texture_sampler(), nullptr);
		vkDestroyImageView(vulkan_device->get_device(), models.at(id)->get_texture_img_view(), nullptr);
		vkDestroyImage(vulkan_device->get_device(), models.at(id)->get_texture_img(), nullptr);
		vkFreeMemory(vulkan_device->get_device(), models.at(id)->get_texture_memory(), nullptr);

		models.at(id)->assign_texture(path);
	}
	void Engine::switch_animated_rotation(const int id)
	{
		models.at(id)->switch_animated_rotation();
	}
	void Engine::create_camera()
	{
		Free_camera new_camera(aspect_ratio);
		cameras.emplace_back(new_camera);
		active_camera = &cameras.at(camera_index);
	}
	void Engine::create_camera(const float x, const float y, const float z)
	{
		Free_camera new_camera(x, y, z, aspect_ratio);
		cameras.emplace_back(new_camera);
		active_camera = &cameras.at(camera_index);
	}
	void Engine::translate_camera(const int id, const float x, const float y, const float z)
	{
		cameras.at(id).set_translation_vector(x, y, z);
	}
	void Engine::set_camera_position(const int id, const float x, const float y, const float z)
	{
		cameras.at(id).set_position_vector(x, y, z);
	}
	void Engine::rotate_camera(const int id, const float yaw, const float pitch, const float roll)
	{
		cameras.at(id).set_rotation(yaw, pitch, roll);
		cameras.at(id).update();
	}
	void Engine::change_fov(const int id, const float fov)
	{
		cameras.at(id).set_fov(fov);
		cameras.at(id).update();
	}
	void Engine::create_model(const std::string& model_path)
	{
		models.emplace_back(std::make_unique<Model>(model_path, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_model(const std::string& model_path, const float x, const float y, const float z)
	{
		models.emplace_back(std::make_unique<Model>(model_path, x, y, z, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_model(const std::string& model_path, const std::string& tex_path)
	{
		models.emplace_back(std::make_unique<Model>(model_path, tex_path, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_model(const std::string& model_path, const std::string& tex_path, const float x, const float y, const float z)
	{
		models.emplace_back(std::make_unique<Model>(model_path, tex_path, x, y, z, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_sphere(const float radious)
	{
		models.emplace_back(std::make_unique<Sphere>(radious, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_sphere(const float radious, const float x, const float y, const float z)
	{
		models.emplace_back(std::make_unique<Sphere>(radious, x, y, z, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();

	}
	void Engine::create_sphere(const float radious, const std::string& tex_path, const float x, const float y, const float z)
	{
		models.emplace_back(std::make_unique<Sphere>(radious, tex_path, x, y, z, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_sphere(const float radious, const std::string& tex_path)
	{
		models.emplace_back(std::make_unique<Sphere>(radious, tex_path, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_plane(const float width, const float height)
	{
		models.emplace_back(std::make_unique<Plane>(width, height, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_plane(const float width, const float height, const float x, const float y, const float z)
	{
		models.emplace_back(std::make_unique<Plane>(width, height, x, y, z, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_plane(const float width, const float height, const std::string& tex_path)
	{
		models.emplace_back(std::make_unique<Plane>(width, height, tex_path, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_plane(const float width, const float height, const std::string& tex_path, const float x, const float y, const float z)
	{
		models.emplace_back(std::make_unique<Plane>(width, height, tex_path, x, y, z, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_box(const float width, const float height, const float length)
	{
		models.emplace_back(std::make_unique<Box>(width, height, length, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_box(const float width, const float height, const float length, const float x, const float y, const float z)
	{
		models.emplace_back(std::make_unique<Box>(width, height, length, x, y, z, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_box(const float width, const float height, const float length, const std::string& tex_path)
	{
		models.emplace_back(std::make_unique<Box>(width, height, length, tex_path, swap_chain_images.size(),
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::create_box(const float width, const float height, const float length, const std::string& tex_path, const float x, const float y, const float z)
	{
		models.emplace_back(std::make_unique<Box>(width, height, length, tex_path, x, y, z, swap_chain_images.size(), 
			command_pool, graphics_queue, descriptor_set_layout, vulkan_device));
		models.back()->init_model();
	}
	void Engine::translate_model(const int id, const float x, const float y, const float z)
	{
		models.at(id)->translate(x, y, z);
	}
	void Engine::rotate_model(const int id, const float x, const float y, const float z)
	{
		models.at(id)->rotate(x, y, z);
	}
	void Engine::scale_model(const int id, const float x, const float y, const float z)
	{
		models.at(id)->scale(x, y, z);
	}
	bool Engine::check_valid_layer_supp()
	{
		uint32_t layer_count;
		vkEnumerateInstanceLayerProperties(&layer_count, nullptr);
		std::vector<VkLayerProperties> available_layers(layer_count);
		vkEnumerateInstanceLayerProperties(&layer_count, available_layers.data());
		for (const auto& layer : validation_layers)
		{
			bool layer_found = false;
			for (const auto& layer_properties : available_layers)
			{
				if (strcmp(layer, layer_properties.layerName) == 0)
				{
					layer_found = true;
					break;
				}
			}
			if (!layer_found)
				return false;
		}
		return true;
	}
	std::vector<const char*> Engine::get_required_ext()
	{
		uint32_t extension_count{};
		const char** glfw_extensions = glfwGetRequiredInstanceExtensions(&extension_count);
		std::vector<const char*> ext(glfw_extensions, extension_count + glfw_extensions);
		if (enable_validation_layers)
			ext.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
		return ext;

	}
	void Engine::create_instance()
	{
		VkApplicationInfo app_info{};
		app_info.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
		app_info.pApplicationName = app_info.pEngineName = app_name.c_str();
		app_info.engineVersion = app_info.apiVersion = app_info.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
		VkInstanceCreateInfo create_info{};
		auto glfw_extensions = get_required_ext();
		create_info.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		create_info.pApplicationInfo = &app_info;
		vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, nullptr);
		supported_extensions.resize(supported_extension_count);
		vkEnumerateInstanceExtensionProperties(nullptr, &supported_extension_count, supported_extensions.data());

		create_info.enabledExtensionCount = static_cast<uint32_t>(glfw_extensions.size());
		create_info.ppEnabledExtensionNames = glfw_extensions.data();
		if (enable_validation_layers)
		{
			create_info.enabledLayerCount = static_cast<uint32_t>(validation_layers.size());
			create_info.ppEnabledLayerNames = validation_layers.data();
		}
		if (vkCreateInstance(&create_info, nullptr, &instance) != VK_SUCCESS)
			throw std::runtime_error("Unable to create Vulkan Instance object!\n");
	}
	void Engine::debug_messenger_setup()
	{
		if (enable_validation_layers)
		{
			VkDebugUtilsMessengerCreateInfoEXT info{};
			info.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			info.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			info.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT
				| VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			info.pfnUserCallback = debug_callback;
			if (create_debug_utils_messenger_EXT(instance, &info, nullptr, &messenger) != VK_SUCCESS)
				throw std::runtime_error("Failed to setup debug messenger!\n");
		}


	}
	VKAPI_ATTR VkBool32 VKAPI_CALL Engine::debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
		VkDebugUtilsMessageTypeFlagsEXT messageType,
		const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
		void* pUserData)
	{
		std::cerr << "Current validation layer: " << pCallbackData->pMessage << '\n';
		return VK_FALSE;
	}
	void Engine::framebuffer_resize(GLFWwindow* window, int width, int height)
	{
		auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
		app->framebuffer_resized = true;
	}
	void Engine::camera_switch_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
	{
		auto app = reinterpret_cast<Engine*>(glfwGetWindowUserPointer(window));
		if (key == GLFW_KEY_2 && action == GLFW_RELEASE)
		{
			app->camera_index = ++(app->camera_index) % app->cameras.size();
			app->active_camera = &(app->cameras.at(app->camera_index));
		}
		if (key == GLFW_KEY_F && action == GLFW_PRESS)
		{
			app->toogle_wireframe();
		}
	}
	void Engine::create_swap_chain()
	{
		Swap_chain_support_details support_detail = vulkan_device->query_swap_chain_support(vulkan_device->get_physical_device());
		VkPresentModeKHR pres_mode = choose_presentation_mode(support_detail.presentation_modes);
		VkSurfaceFormatKHR surf_format = choose_surface_format(support_detail.formats);
		VkExtent2D extent = choose_swap_extend(support_detail.capabilities);
		uint32_t image_count = support_detail.capabilities.minImageCount + 1;
		if (support_detail.capabilities.maxImageCount > 0 && image_count > support_detail.capabilities.maxImageCount)
			image_count = support_detail.capabilities.maxImageCount;
		VkSwapchainCreateInfoKHR swap_info{};
		swap_info.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
		swap_info.surface = surface;
		swap_info.minImageCount = image_count;
		swap_info.imageExtent = swap_chain_extent = extent;
		swap_info.imageFormat = swap_chain_image_format = surf_format.format;
		swap_info.imageColorSpace = surf_format.colorSpace;
		swap_info.imageArrayLayers = 1;
		swap_info.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		swap_info.preTransform = support_detail.capabilities.currentTransform;
		swap_info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		swap_info.clipped = VK_TRUE;
		swap_info.presentMode = pres_mode;
		swap_info.oldSwapchain = VK_NULL_HANDLE;
		Queue_family_indecies ind = vulkan_device->find_queue_family_indicies(vulkan_device->get_physical_device());
		std::array<uint32_t, 2> queue_family_indecies = { ind.graphics_family.value(), ind.present_family.value() };
		if (ind.graphics_family != ind.present_family)
		{
			swap_info.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
			swap_info.queueFamilyIndexCount = queue_family_indecies.size();
			swap_info.pQueueFamilyIndices = queue_family_indecies.data();
		}
		else
			swap_info.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		if (vkCreateSwapchainKHR(vulkan_device->get_device(), &swap_info, nullptr, &swap_chain) != VK_SUCCESS)
			throw std::runtime_error("Swap chain creation failed!\n");


		vkGetSwapchainImagesKHR(vulkan_device->get_device(), swap_chain, &image_count, nullptr);
		swap_chain_images.resize(image_count);
		vkGetSwapchainImagesKHR(vulkan_device->get_device(), swap_chain, &image_count, swap_chain_images.data());

		aspect_ratio = static_cast<float>(swap_chain_extent.width) / static_cast<float>(swap_chain_extent.height);

	}
	void Engine::create_image_views()
	{
		swap_chain_img_views.resize(swap_chain_images.size());
		for (int i = 0; i < swap_chain_images.size(); ++i)
		{
			swap_chain_img_views.at(i) = create_image_view(swap_chain_images.at(i), swap_chain_image_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		}
	}
	void Engine::create_descriptor_set_layout()
	{
		VkDescriptorSetLayoutBinding ubo_binding{};
		ubo_binding.binding = 0;
		ubo_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		ubo_binding.descriptorCount = 1;
		ubo_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
		VkDescriptorSetLayoutBinding sampler_binding{};
		sampler_binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		sampler_binding.binding = sampler_binding.descriptorCount = 1;
		sampler_binding.pImmutableSamplers = nullptr;
		sampler_binding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

		std::array<VkDescriptorSetLayoutBinding, 2> bindings = { ubo_binding, sampler_binding };

		VkDescriptorSetLayoutCreateInfo layout_info{};
		layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		layout_info.bindingCount = static_cast<uint32_t>(bindings.size());
		layout_info.pBindings = bindings.data();
		if (vkCreateDescriptorSetLayout(vulkan_device->get_device(), &layout_info, nullptr, &descriptor_set_layout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create descriptor set layout!\n");


	}
	void Engine::create_graphics_pipeline()
	{
		//auto vertex_shader = read_shader_file(R"(src\vert.spv)");
		//auto fragment_shader = read_shader_file(poly_mode.second);

		Shader vertex_shader(R"(src\vert.spv)", vulkan_device->get_device()),
			fragment_shader(poly_mode.second, vulkan_device->get_device());

		VkShaderModule vertex_module = vertex_shader.create_shader_module(),
			fragment_module = fragment_shader.create_shader_module();

		VkPipelineShaderStageCreateInfo pipeline_vertex_info{}, pipeline_fragment_info{};
		pipeline_vertex_info.sType = pipeline_fragment_info.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
		pipeline_vertex_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		pipeline_vertex_info.module = vertex_module;
		pipeline_vertex_info.pName = pipeline_fragment_info.pName = "main";
		pipeline_fragment_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		pipeline_fragment_info.module = fragment_module;
		VkPipelineShaderStageCreateInfo pipeline_infos[] = { pipeline_vertex_info, pipeline_fragment_info };

		VkPipelineVertexInputStateCreateInfo vertex_input_info{};
		auto bindings_description = Vertex::get_binding_description();
		auto attribute_desc = Vertex::get_attribute_descriptions();
		vertex_input_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
		vertex_input_info.pVertexAttributeDescriptions = attribute_desc.data();
		vertex_input_info.pVertexBindingDescriptions = &bindings_description;
		vertex_input_info.vertexAttributeDescriptionCount = static_cast<uint32_t>(attribute_desc.size());
		vertex_input_info.vertexBindingDescriptionCount = 1;

		
		VkPipelineInputAssemblyStateCreateInfo assembly_info{};
		assembly_info.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
		assembly_info.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		assembly_info.primitiveRestartEnable = VK_FALSE;

		VkViewport viewport{};
		viewport.x = viewport.y = 0.0f;
		viewport.height = static_cast<float>(swap_chain_extent.height);
		viewport.width = static_cast<float>(swap_chain_extent.width);
		viewport.minDepth = 0.0f;
		viewport.maxDepth = 1.0f;

		VkRect2D scissors{};
		scissors.offset = { 0, 0 };
		scissors.extent = swap_chain_extent;
		VkPipelineViewportStateCreateInfo viewport_info{};
		viewport_info.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
		viewport_info.viewportCount = viewport_info.scissorCount = 1;
		viewport_info.pScissors = &scissors;
		viewport_info.pViewports = &viewport;

		VkPipelineRasterizationStateCreateInfo rasterizer{};
		rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
		rasterizer.depthClampEnable = rasterizer.rasterizerDiscardEnable = rasterizer.depthBiasEnable = VK_FALSE;
		rasterizer.polygonMode = poly_mode.first;
		rasterizer.lineWidth = 0.5f;
		rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
		rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;

		VkPipelineMultisampleStateCreateInfo multisampling{};
		multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
		multisampling.sampleShadingEnable = VK_TRUE;
		multisampling.rasterizationSamples = vulkan_device->get_msaa_samples();
		multisampling.minSampleShading = 0.2f;

		VkPipelineColorBlendAttachmentState colour_blending_att{};
		colour_blending_att.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT
			| VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
		colour_blending_att.blendEnable = VK_FALSE;

		VkPipelineColorBlendStateCreateInfo colour_blending{};
		colour_blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
		colour_blending.pAttachments = &colour_blending_att;
		colour_blending.attachmentCount = 1;
		colour_blending.logicOpEnable = VK_FALSE;

		VkDynamicState dynamic_states[] = {
			VK_DYNAMIC_STATE_VIEWPORT,
			VK_DYNAMIC_STATE_LINE_WIDTH
		};
		VkPipelineDynamicStateCreateInfo dynamic_state{};
		dynamic_state.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
		dynamic_state.pDynamicStates = dynamic_states;
		dynamic_state.dynamicStateCount = 2;

		VkPipelineLayoutCreateInfo layout_info{};
		layout_info.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
		layout_info.setLayoutCount = 1;
		layout_info.pSetLayouts = &descriptor_set_layout;
		layout_info.pushConstantRangeCount = 0;
		layout_info.pPushConstantRanges = nullptr;
		if (vkCreatePipelineLayout(vulkan_device->get_device(), &layout_info, nullptr, &pipeline_layout) != VK_SUCCESS)
			throw std::runtime_error("Failed to create pipeline layout!\n");

		VkPipelineDepthStencilStateCreateInfo depth_stencil_info{};
		depth_stencil_info.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
		depth_stencil_info.depthTestEnable = depth_stencil_info.depthWriteEnable = VK_TRUE;
		depth_stencil_info.depthCompareOp = VK_COMPARE_OP_LESS;
		depth_stencil_info.depthBoundsTestEnable = depth_stencil_info.stencilTestEnable = VK_FALSE;

		VkGraphicsPipelineCreateInfo graphics_pipeline_info{};
		graphics_pipeline_info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
		graphics_pipeline_info.stageCount = 2;
		graphics_pipeline_info.pStages = pipeline_infos;
		graphics_pipeline_info.pVertexInputState = &vertex_input_info;
		graphics_pipeline_info.pRasterizationState = &rasterizer;
		graphics_pipeline_info.pInputAssemblyState = &assembly_info;
		graphics_pipeline_info.pViewportState = &viewport_info;
		graphics_pipeline_info.pMultisampleState = &multisampling;
		graphics_pipeline_info.pColorBlendState = &colour_blending;
		graphics_pipeline_info.layout = pipeline_layout;
		graphics_pipeline_info.subpass = 0;
		graphics_pipeline_info.renderPass = render_pass;
		graphics_pipeline_info.pDepthStencilState = &depth_stencil_info;
		if (vkCreateGraphicsPipelines(vulkan_device->get_device(), VK_NULL_HANDLE, 1, &graphics_pipeline_info, nullptr, &pipeline) != VK_SUCCESS)
			throw std::runtime_error("Failed to create graphics pipeline!\n");


		vkDestroyShaderModule(vulkan_device->get_device(), vertex_module, nullptr);
		vkDestroyShaderModule(vulkan_device->get_device(), fragment_module, nullptr);
	}
	void Engine::create_render_passes()
	{
		VkAttachmentDescription colour_att{};
		colour_att.format = swap_chain_image_format;
		colour_att.samples = vulkan_device->get_msaa_samples();
		colour_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		colour_att.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colour_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colour_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colour_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colour_att.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentReference colour_att_ref{};
		colour_att_ref.attachment = 0;
		colour_att_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription depth_att{};
		depth_att.format = find_depth_format();
		depth_att.samples = vulkan_device->get_msaa_samples();
		depth_att.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		depth_att.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_att.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depth_att.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depth_att.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depth_att.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentReference depth_att_ref{};
		depth_att_ref.attachment = 1;
		depth_att_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription colour_att_resolve{};
		colour_att_resolve.format = swap_chain_image_format;
		colour_att_resolve.samples = VK_SAMPLE_COUNT_1_BIT;
		colour_att_resolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		colour_att_resolve.loadOp = colour_att_resolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		colour_att_resolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		colour_att_resolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		colour_att_resolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		VkAttachmentReference colour_att_resolve_ref{};
		colour_att_resolve_ref.attachment = 2;
		colour_att_resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;



		VkSubpassDescription subpass{};
		subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
		subpass.colorAttachmentCount = 1;
		subpass.pColorAttachments = &colour_att_ref;
		subpass.pDepthStencilAttachment = &depth_att_ref;
		subpass.pResolveAttachments = &colour_att_resolve_ref;
		std::array<VkAttachmentDescription, 3> attachments = { colour_att, depth_att, colour_att_resolve };
		VkRenderPassCreateInfo render_pass_info{};
		render_pass_info.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
		render_pass_info.attachmentCount = static_cast<uint32_t>(attachments.size());
		render_pass_info.subpassCount = 1;
		render_pass_info.pAttachments = attachments.data();
		render_pass_info.pSubpasses = &subpass;
		VkSubpassDependency subpass_dep{};
		subpass_dep.srcSubpass = VK_SUBPASS_EXTERNAL;
		subpass_dep.dstSubpass = subpass_dep.srcAccessMask = 0;
		subpass_dep.srcStageMask = subpass_dep.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		subpass_dep.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		render_pass_info.dependencyCount = 1;
		render_pass_info.pDependencies = &subpass_dep;
		if (vkCreateRenderPass(vulkan_device->get_device(), &render_pass_info, nullptr, &render_pass) != VK_SUCCESS)
			throw std::runtime_error("Failed to create render pass!\n");

	}
	void Engine::create_framebuffers()
	{
		swap_chain_framebuffers.resize(swap_chain_img_views.size());
		for (int i = 0; i < swap_chain_img_views.size(); ++i)
		{
			std::array<VkImageView, 3> attachments = { colour_img_view, depth_img_view, swap_chain_img_views.at(i) };
			VkFramebufferCreateInfo framebuffer_info{};
			framebuffer_info.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
			framebuffer_info.pAttachments = attachments.data();
			framebuffer_info.attachmentCount = static_cast<uint32_t>(attachments.size());
			framebuffer_info.layers = 1;
			framebuffer_info.renderPass = render_pass;
			framebuffer_info.height = swap_chain_extent.height;
			framebuffer_info.width = swap_chain_extent.width;
			if (vkCreateFramebuffer(vulkan_device->get_device(), &framebuffer_info, nullptr, &swap_chain_framebuffers.at(i)) != VK_SUCCESS)
				throw std::runtime_error("Failed to create framebuffer!\n");

		}
	}
	void Engine::create_command_pool()
	{
		auto family_indecies = vulkan_device->find_queue_family_indicies(vulkan_device->get_physical_device());
		VkCommandPoolCreateInfo pool_info{};
		pool_info.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		pool_info.queueFamilyIndex = family_indecies.graphics_family.value();
		if (vkCreateCommandPool(vulkan_device->get_device(), &pool_info, nullptr, &command_pool) != VK_SUCCESS)
			throw std::runtime_error("Failed to create command pool!\n");

	}
	VkImageView Engine::create_image_view(VkImage image, VkFormat format, VkImageAspectFlags aspect_flags, uint32_t mip_levels)
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
		if (vkCreateImageView(vulkan_device->get_device(), &info, nullptr, &img_view) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture image view!\n");
		return img_view;
	}
	void Engine::create_image(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags flags, VkMemoryPropertyFlags properties, VkImage& img, VkDeviceMemory& mem, uint32_t mip_levels, VkSampleCountFlagBits num_samples)
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
		if (vkCreateImage(vulkan_device->get_device(), &img_info, nullptr, &img) != VK_SUCCESS)
			throw std::runtime_error("Failed to create texture image!\n");
		VkMemoryRequirements mem_req{};
		vkGetImageMemoryRequirements(vulkan_device->get_device(), img, &mem_req);
		VkMemoryAllocateInfo malloc_info{};
		malloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
		malloc_info.allocationSize = mem_req.size;
		malloc_info.memoryTypeIndex = find_memory_type(mem_req.memoryTypeBits, properties);
		if (vkAllocateMemory(vulkan_device->get_device(), &malloc_info, nullptr, &mem) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate texture memory!\n");
		vkBindImageMemory(vulkan_device->get_device(), img, mem, 0);
	}
	VkCommandBuffer Engine::begin_single_time_commands()
	{
		VkCommandBufferAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandBufferCount = 1;
		alloc_info.commandPool = command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;

		VkCommandBuffer command_buffer;
		vkAllocateCommandBuffers(vulkan_device->get_device(), &alloc_info, &command_buffer);
		VkCommandBufferBeginInfo begin_info{};
		begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		vkBeginCommandBuffer(command_buffer, &begin_info);

		return command_buffer;
	}
	void Engine::end_single_time_commands(VkCommandBuffer command_buffer)
	{
		vkEndCommandBuffer(command_buffer);
		VkSubmitInfo submit_info{};
		submit_info.commandBufferCount = 1;
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submit_info.pCommandBuffers = &command_buffer;
		vkQueueSubmit(graphics_queue, 1, &submit_info, VK_NULL_HANDLE);
		vkDeviceWaitIdle(vulkan_device->get_device());
		vkFreeCommandBuffers(vulkan_device->get_device(), command_pool, 1, &command_buffer);
	}
	void Engine::transition_image_layout(VkImage img, VkFormat format, VkImageLayout old_layout, VkImageLayout new_layout, uint32_t mip_levels)
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
	void Engine::create_command_buffers()
	{
		command_buffers.resize(swap_chain_framebuffers.size());
		VkCommandBufferAllocateInfo alloc_info{};
		alloc_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		alloc_info.commandBufferCount = static_cast<uint32_t>(command_buffers.size());
		alloc_info.commandPool = command_pool;
		alloc_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		if (vkAllocateCommandBuffers(vulkan_device->get_device(), &alloc_info, command_buffers.data()) != VK_SUCCESS)
			throw std::runtime_error("Failed to allocate command buffers!\n");

		for (int i = 0; i < command_buffers.size(); ++i)
		{
			VkCommandBufferBeginInfo buffer_begin_info{};
			buffer_begin_info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
			if (vkBeginCommandBuffer(command_buffers.at(i), &buffer_begin_info) != VK_SUCCESS)
				throw std::runtime_error("Failed to begin recording command buffer!\n");
			VkRenderPassBeginInfo render_pass_begin{};
			render_pass_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
			render_pass_begin.renderPass = render_pass;
			render_pass_begin.framebuffer = swap_chain_framebuffers.at(i);
			render_pass_begin.renderArea.offset = { 0, 0 };
			render_pass_begin.renderArea.extent = swap_chain_extent;
			std::array<VkClearValue, 2> clear_values{};
			clear_values.at(0).color = { 0.0f, 0.0f, 0.0f, 1.0f };
			clear_values.at(1).depthStencil = { 1.0f, 0 };
			render_pass_begin.clearValueCount = static_cast<uint32_t>(clear_values.size());
			render_pass_begin.pClearValues = clear_values.data();
			vkCmdBeginRenderPass(command_buffers.at(i), &render_pass_begin, VK_SUBPASS_CONTENTS_INLINE);
			vkCmdBindPipeline(command_buffers.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
			for (const auto& model : models)
			{
				VkBuffer vertex_buffers[] = { model->get_vertex_buffer() };
				VkDeviceSize offsets[] = { 0 };
				vkCmdBindVertexBuffers(command_buffers.at(i), 0, 1, vertex_buffers, offsets);
				vkCmdBindIndexBuffer(command_buffers.at(i), model->get_index_buffer(), 0, VK_INDEX_TYPE_UINT32);
				vkCmdBindDescriptorSets(command_buffers.at(i), VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline_layout,
					0, 1, model->get_descriptor_sets().data(), 0, nullptr);
				vkCmdDrawIndexed(command_buffers.at(i), model->get_indicies_size(), 1, 0, 0, 0);

			}
			vkCmdEndRenderPass(command_buffers.at(i));
			if (vkEndCommandBuffer(command_buffers.at(i)) != VK_SUCCESS)
				throw std::runtime_error("Failed to end command buffer recording!\n");

		}

	}
	void Engine::create_semaphores_and_fences()
	{
		image_available_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		rendering_finished_semaphores.resize(MAX_FRAMES_IN_FLIGHT);
		in_flight_fences.resize(MAX_FRAMES_IN_FLIGHT);
		images_in_flight.resize(swap_chain_images.size(), VK_NULL_HANDLE);
		VkSemaphoreCreateInfo semaphore_info{};
		semaphore_info.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
		VkFenceCreateInfo fence_info{};
		fence_info.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
		fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT;
		for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
			if (vkCreateSemaphore(vulkan_device->get_device(), &semaphore_info, nullptr, &image_available_semaphores.at(i)) != VK_SUCCESS ||
				vkCreateSemaphore(vulkan_device->get_device(), &semaphore_info, nullptr, &rendering_finished_semaphores.at(i)) != VK_SUCCESS ||
				vkCreateFence(vulkan_device->get_device(), &fence_info, nullptr, &in_flight_fences.at(i)) != VK_SUCCESS)
				throw std::runtime_error("Failed to create sync objects for a frame!\n");

	}
	void Engine::create_colour_resources()
	{
		VkFormat colour_format = swap_chain_image_format;
		create_image(swap_chain_extent.width, swap_chain_extent.height, colour_format,
			VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, colour_img, colour_mem, 1, vulkan_device->get_msaa_samples());
		colour_img_view = create_image_view(colour_img, colour_format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
		transition_image_layout(colour_img, colour_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL, 1);
	}
	void Engine::create_depth_resources()
	{
		VkFormat depth_format = find_depth_format();
		create_image(swap_chain_extent.width, swap_chain_extent.height, depth_format, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depth_img, depth_mem, 1, vulkan_device->get_msaa_samples());
		depth_img_view = create_image_view(depth_img, depth_format, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		transition_image_layout(depth_img, depth_format, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

	}
	void Engine::clean_swap_chain()
	{
		vkDestroyImageView(vulkan_device->get_device(), colour_img_view, nullptr);
		vkDestroyImage(vulkan_device->get_device(), colour_img, nullptr);
		vkFreeMemory(vulkan_device->get_device(), colour_mem, nullptr);		
		vkDestroyImageView(vulkan_device->get_device(), depth_img_view, nullptr);
		vkDestroyImage(vulkan_device->get_device(), depth_img, nullptr);
		vkFreeMemory(vulkan_device->get_device(), depth_mem, nullptr);
		for (const auto& framebuffer : swap_chain_framebuffers)
			vkDestroyFramebuffer(vulkan_device->get_device(), framebuffer, nullptr);
		vkFreeCommandBuffers(vulkan_device->get_device(), command_pool, static_cast<uint32_t>(command_buffers.size()), command_buffers.data());
		vkDestroyPipeline(vulkan_device->get_device(), pipeline, nullptr);
		vkDestroyPipelineLayout(vulkan_device->get_device(), pipeline_layout, nullptr);
		vkDestroyRenderPass(vulkan_device->get_device(), render_pass, nullptr);
		for (const auto& view : swap_chain_img_views)
			vkDestroyImageView(vulkan_device->get_device(), view, nullptr);
		vkDestroySwapchainKHR(vulkan_device->get_device(), swap_chain, nullptr);
		for (const auto& model : models)
		{
			auto uniform_buffers = model->get_uniform_buffers();
			for (int i = 0; i < uniform_buffers.size(); ++i)
			{
				vkDestroyBuffer(vulkan_device->get_device(), uniform_buffers.at(i), nullptr);
				vkFreeMemory(vulkan_device->get_device(), model->get_uniform_buffer_memory(i), nullptr);
			}
			vkDestroyDescriptorPool(vulkan_device->get_device(), model->get_descriptor_pool(), nullptr);
		}
	}
	void Engine::update_uniform_buffer(uint32_t index)
	{
		static auto start_time = std::chrono::high_resolution_clock::now();
		auto current_time = std::chrono::high_resolution_clock::now();
		float time = std::chrono::duration<float, std::chrono::seconds::period>(current_time - start_time).count();
		
		for (const auto &model : models)
		{
			Uniform_buffer_object ubo{};
			if (model->get_animation_state())
				ubo.model = glm::rotate(model->get_model_matrix(), glm::radians(90.0f) * time, glm::vec3(0.0f, 1.0f, 0.0f));
			else
				ubo.model = model->get_model_matrix();
			ubo.view = active_camera->get_view_matrix();
			ubo.proj = active_camera->get_projection_matrix();
			ubo.proj[1][1] *= -1;

			void* data;
			vkMapMemory(vulkan_device->get_device(), model->get_uniform_buffer_memory(index), 0, sizeof(ubo), 0, &data);
			memcpy(data, &ubo, sizeof(ubo));
			vkUnmapMemory(vulkan_device->get_device(), model->get_uniform_buffer_memory(index));
		}
	}
	void Engine::draw_frame()
	{
		vkWaitForFences(vulkan_device->get_device(), 1, &in_flight_fences.at(current_frame), VK_TRUE, std::numeric_limits<uint64_t>::max());
		uint32_t image_index{};
		vkAcquireNextImageKHR(vulkan_device->get_device(), swap_chain, std::numeric_limits<uint64_t>::max(), image_available_semaphores.at(current_frame), VK_NULL_HANDLE, &image_index);

		if (images_in_flight.at(image_index) != VK_NULL_HANDLE)
			vkWaitForFences(vulkan_device->get_device(), 1, &images_in_flight.at(image_index), VK_TRUE, std::numeric_limits<uint64_t>::max());

		images_in_flight.at(image_index) = in_flight_fences.at(current_frame);

		update_uniform_buffer(image_index);

		VkSubmitInfo submit_info{};
		submit_info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		VkSemaphore wait_semaphores[] = { image_available_semaphores.at(current_frame) };
		VkPipelineStageFlags wait_stages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
		submit_info.pCommandBuffers = &command_buffers[image_index];
			submit_info.commandBufferCount = 1;
			submit_info.waitSemaphoreCount = 1;
			submit_info.pWaitSemaphores = wait_semaphores;
			submit_info.pWaitDstStageMask = wait_stages;
			VkSemaphore signal_semaphores[] = { rendering_finished_semaphores.at(current_frame) };
			submit_info.signalSemaphoreCount = 1;
			submit_info.pSignalSemaphores = signal_semaphores;
			vkResetFences(vulkan_device->get_device(), 1, &in_flight_fences.at(current_frame));
			if (vkQueueSubmit(graphics_queue, 1, &submit_info, in_flight_fences.at(current_frame)) != VK_SUCCESS)
				throw std::runtime_error("Failed to submit draw command buffer!\n");
		VkPresentInfoKHR present_info{};
		VkSwapchainKHR swap_chains[] = { swap_chain };
		present_info.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
		present_info.waitSemaphoreCount = 1;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = swap_chains;
		present_info.pWaitSemaphores = signal_semaphores;
		present_info.pImageIndices = &image_index;
		VkResult result = vkQueuePresentKHR(present_queue, &present_info);
		if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebuffer_resized)
		{
			framebuffer_resized = false;
			recreate_swap_chain();
		}
		else if (result != VK_SUCCESS)
			throw std::runtime_error("Failed to acquire swap chain image!\n");

		current_frame = (current_frame + 1) % MAX_FRAMES_IN_FLIGHT;
	}
	void Engine::recreate_swap_chain()
	{
		int width = 0, height = 0;
		while (height == 0 || width == 0)
		{
			glfwGetFramebufferSize(window, &width, &height);
			glfwWaitEvents();
		}
		vkDeviceWaitIdle(vulkan_device->get_device());
		clean_swap_chain();
		create_swap_chain();
		for (auto& camera : cameras)
			camera.set_aspect_ratio(aspect_ratio);
		create_image_views();
		create_render_passes();
		create_graphics_pipeline();
		create_colour_resources();
		create_depth_resources();
		create_framebuffers();
		for (const auto &model: models)
			model->recreate_swap_chain_elements();
		create_command_buffers();

	}
	void Engine::process_input()
	{
		float speed = active_camera->get_speed() * delta_time,
			angle = 45.0f * delta_time;
			if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
				active_camera->walk(speed);

			if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
				active_camera->strafe(-speed);

			if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) 
				active_camera->walk(-speed);

			if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
				active_camera->strafe(+speed);

			if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
				active_camera->lift(speed);

			if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
				active_camera->lift(-speed);

			if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
				active_camera->rotate(0.0f, -angle);

			if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
				active_camera->rotate(0.0f, angle);

			if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
				active_camera->rotate(angle, 0.0f);

			if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
				active_camera->rotate(-angle, 0.0f);

			if (glfwGetKey(window, GLFW_KEY_O) == GLFW_PRESS)
				active_camera->set_fov(-angle);

			if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
				active_camera->set_fov(angle);
	}
	void Engine::info()
	{
		std::string info = R"(Controls:
W/A/S/D/Q/E - Move camera
O/P - Change FOV		
Arrows - Rotate camera
F - Toogle wireframe
2 - Change camera	
			)";
		std::cout << "ENGINE\n\nN. of cameras: " << cameras.size() << "\nN. of models: " << models.size() << '\n' << info << '\n';

	}
	VkSurfaceFormatKHR Engine::choose_surface_format(const std::vector<VkSurfaceFormatKHR>& formats)
	{
		for (const auto& format : formats)
		{
			if (format.format == VK_FORMAT_B8G8R8A8_UNORM && format.colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR)
				return format;
		}
		return formats.front();
	}

	VkPresentModeKHR Engine::choose_presentation_mode(const std::vector<VkPresentModeKHR>& modes)
	{
		for (const auto& mode : modes)
		{
			if (mode == VK_PRESENT_MODE_MAILBOX_KHR)
				return mode;
		}
		return VK_PRESENT_MODE_FIFO_KHR;
	}

	VkExtent2D Engine::choose_swap_extend(const VkSurfaceCapabilitiesKHR& cap)
	{
		if (cap.currentExtent.width != std::numeric_limits<uint32_t>::max())
		{
			return cap.currentExtent;
		}
		else
		{
			int width, height;
			glfwGetFramebufferSize(window, &width, &height);
			VkExtent2D ext{ static_cast<uint32_t>(width), static_cast<uint32_t>(height) };
			ext.width = std::max(cap.minImageExtent.width, std::min(cap.maxImageExtent.width, ext.width));
			ext.height = std::max(cap.minImageExtent.height, std::min(cap.maxImageExtent.height, ext.height));
			return ext;
		}
	}
	uint32_t Engine::find_memory_type(uint32_t type_filter, VkMemoryPropertyFlags prop)
	{
		VkPhysicalDeviceMemoryProperties mem_prop{};
		vkGetPhysicalDeviceMemoryProperties(vulkan_device->get_physical_device(), &mem_prop);
		for (uint32_t i = 0; i < mem_prop.memoryTypeCount; ++i)
		{
			if (type_filter & (1 << i) && 
				((mem_prop.memoryTypes[i].propertyFlags & prop) == prop))
				return i;
		}
		throw std::runtime_error("Failed to find suitable memory type!\n");
		return uint32_t();
	}
	VkFormat Engine::find_supported_format(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags flags)
	{
		for (auto candidate : candidates)
		{
			VkFormatProperties props;
			vkGetPhysicalDeviceFormatProperties(vulkan_device->get_physical_device(), candidate, &props);
			if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & flags) == flags)
				return candidate;
			else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & flags) == flags)
				return candidate;
		}
		throw std::runtime_error("Failed to find supported format!\n");
	}
	VkFormat Engine::find_depth_format()
	{
		return find_supported_format({VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	}
	bool Engine::has_stencil_component(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
	}
