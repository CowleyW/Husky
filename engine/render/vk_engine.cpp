#include "vk_engine.h"

#include "io/logging.h"
#include "render/callback_handler.h"
#include "render/mesh.h"
#include "render/pipeline_builder.h"
#include "render/shader.h"
#include "render/vk_init.h"
#include "render/vk_types.h"

#include "VkBootstrap.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

Render::VulkanEngine::~VulkanEngine() {
  // Make sure the GPU is not doing anything before we do any cleanup
  vkDeviceWaitIdle(this->device);

  vmaDestroyBuffer(
      this->allocator,
      this->triangle_mesh.vertex_buffer.buffer,
      this->triangle_mesh.vertex_buffer.allocation);
  vmaDestroyAllocator(this->allocator);

  vkDestroyPipeline(this->device, this->mono_pipeline, nullptr);
  vkDestroyPipeline(this->device, this->rainbow_pipeline, nullptr);
  vkDestroyPipeline(this->device, this->mesh_pipeline, nullptr);
  vkDestroyPipelineLayout(this->device, this->pipeline_layout, nullptr);

  vkDestroyCommandPool(this->device, this->command_pool, nullptr);
  vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);

  vkDestroyRenderPass(this->device, this->render_pass, nullptr);

  for (uint32_t i = 0; i < this->swapchain_image_views.size(); i += 1) {
    vkDestroyFramebuffer(this->device, this->frame_buffers[i], nullptr);
    vkDestroyImageView(this->device, this->swapchain_image_views[i], nullptr);
  }

  vkDestroySemaphore(this->device, this->present_semaphore, nullptr);
  vkDestroySemaphore(this->device, this->render_semaphore, nullptr);
  vkDestroyFence(this->device, this->render_fence, nullptr);

  vkDestroyDevice(this->device, nullptr);
  vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
  vkb::destroy_debug_utils_messenger(this->instance, this->debug_messenger);

  vkDestroyInstance(this->instance, nullptr);
}

Err Render::VulkanEngine::init(
    Dimensions dimensions,
    CallbackHandler *handler) {
  this->dimensions = dimensions;
  this->window.init(dimensions);
  this->window.register_callbacks(handler);

  this->init_vulkan();
  this->init_swapchain();
  this->init_commands();
  this->init_default_renderpass();
  this->init_framebuffers();
  this->init_sync_structs();
  this->init_pipelines();
  this->init_allocator();
  this->init_meshes();

  return Err::ok();
}

void Render::VulkanEngine::render() {
  VK_ASSERT(
      vkWaitForFences(this->device, 1, &this->render_fence, true, 1000000000));
  VK_ASSERT(vkResetFences(this->device, 1, &this->render_fence));

  uint32_t next_image_index;
  VK_ASSERT(vkAcquireNextImageKHR(
      this->device,
      this->swapchain,
      1000000000,
      this->present_semaphore,
      nullptr,
      &next_image_index));

  VK_ASSERT(vkResetCommandBuffer(this->main_command_buffer, 0));

  VkCommandBufferBeginInfo cmd_info = VkInit::command_buffer_begin_info();
  VK_ASSERT(vkBeginCommandBuffer(this->main_command_buffer, &cmd_info));

  VkClearValue clear_value = {};
  float b = std::abs(std::sin(this->frame_number / 120.0f));
  clear_value.color = {{b, b, b, 1.0f}};

  VkRenderPassBeginInfo render_pass_info = VkInit::render_pass_begin_info(
      this->render_pass,
      this->frame_buffers[next_image_index],
      &clear_value,
      this->dimensions);

  vkCmdBeginRenderPass(
      this->main_command_buffer,
      &render_pass_info,
      VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(
      this->main_command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->mesh_pipeline);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(
      this->main_command_buffer,
      0,
      1,
      &this->triangle_mesh.vertex_buffer.buffer,
      &offset);
  vkCmdDraw(
      this->main_command_buffer,
      this->triangle_mesh.vertices.size(),
      1,
      0,
      0);

  vkCmdEndRenderPass(this->main_command_buffer);
  VK_ASSERT(vkEndCommandBuffer(this->main_command_buffer));

  VkPipelineStageFlags wait_stage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit_info = VkInit::submit_info(
      &this->present_semaphore,
      &this->render_semaphore,
      &this->main_command_buffer,
      &wait_stage);
  VK_ASSERT(
      vkQueueSubmit(this->graphics_queue, 1, &submit_info, this->render_fence));

  VkPresentInfoKHR present_info = VkInit::present_info(
      &this->swapchain,
      &this->render_semaphore,
      &next_image_index);
  VK_ASSERT(vkQueuePresentKHR(this->graphics_queue, &present_info));

  this->frame_number += 1;
}

void Render::VulkanEngine::resize(Dimensions dimensions) {
}

void Render::VulkanEngine::poll_events() {
  this->window.poll_events();
}

InputMap Render::VulkanEngine::get_inputs() {
  return this->window.get_inputs();
}

void Render::VulkanEngine::init_vulkan() {
  vkb::InstanceBuilder builder;

  auto inst_ret = builder.set_app_name("Husky")
                      .request_validation_layers()
                      .use_default_debug_messenger()
                      .require_api_version(1, 1, 0)
                      .build();

  vkb::Instance vkb_inst = inst_ret.value();

  this->instance = vkb_inst.instance;
  this->debug_messenger = vkb_inst.debug_messenger;

  this->surface = this->window.create_surface(this->instance);

  vkb::PhysicalDeviceSelector selector(vkb_inst);
  vkb::PhysicalDevice gpu = selector.set_minimum_version(1, 1)
                                .set_surface(this->surface)
                                .select()
                                .value();

  vkb::DeviceBuilder device_builder(gpu);
  vkb::Device device = device_builder.build().value();

  this->gpu = gpu.physical_device;
  this->device = device.device;

  this->graphics_queue = device.get_queue(vkb::QueueType::graphics).value();
  this->graphics_queue_family =
      device.get_queue_index(vkb::QueueType::graphics).value();
}

void Render::VulkanEngine::init_swapchain() {
  vkb::SwapchainBuilder swapchain_builder(
      this->gpu,
      this->device,
      this->surface);

  vkb::Swapchain vkb_swapchain =
      swapchain_builder.use_default_format_selection()
          .set_desired_present_mode(VK_PRESENT_MODE_FIFO_KHR)
          .set_desired_extent(this->dimensions.width, this->dimensions.height)
          .build()
          .value();

  this->swapchain = vkb_swapchain.swapchain;
  this->swapchain_images = vkb_swapchain.get_images().value();
  this->swapchain_image_views = vkb_swapchain.get_image_views().value();
  this->swapchain_format = vkb_swapchain.image_format;
}

void Render::VulkanEngine::init_commands() {
  VkCommandPoolCreateInfo command_pool_info = VkInit::command_pool_create_info(
      this->graphics_queue_family,
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  VK_ASSERT(vkCreateCommandPool(
      this->device,
      &command_pool_info,
      nullptr,
      &this->command_pool));

  VkCommandBufferAllocateInfo alloc_info = VkInit::command_buffer_allocate_info(
      this->command_pool,
      1,
      VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VK_ASSERT(vkAllocateCommandBuffers(
      this->device,
      &alloc_info,
      &this->main_command_buffer));
}

void Render::VulkanEngine::init_default_renderpass() {
  VkAttachmentDescription color_attachment =
      VkInit::color_attachment(this->swapchain_format);

  VkAttachmentReference color_attachment_ref = VkInit::color_attachment_ref();

  VkSubpassDescription subpass = VkInit::subpass_description(
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      &color_attachment_ref);

  VkRenderPassCreateInfo render_pass_info =
      VkInit::render_pass_create_info(&color_attachment, &subpass);

  VK_ASSERT(vkCreateRenderPass(
      this->device,
      &render_pass_info,
      nullptr,
      &this->render_pass));
}

void Render::VulkanEngine::init_framebuffers() {
  VkFramebufferCreateInfo frame_buffer_info =
      VkInit::frame_buffer_create_info(this->render_pass, this->dimensions);

  const uint32_t num_images = this->swapchain_images.size();
  this->frame_buffers = std::vector<VkFramebuffer>(num_images);

  for (uint32_t i = 0; i < num_images; i += 1) {
    frame_buffer_info.pAttachments = &this->swapchain_image_views[i];

    VK_ASSERT(vkCreateFramebuffer(
        this->device,
        &frame_buffer_info,
        nullptr,
        &this->frame_buffers[i]));
  }
}

void Render::VulkanEngine::init_sync_structs() {
  VkFenceCreateInfo fence_create_info = VkInit::fence_create_info();

  VK_ASSERT(vkCreateFence(
      this->device,
      &fence_create_info,
      nullptr,
      &this->render_fence));

  VkSemaphoreCreateInfo semaphore_create_info = VkInit::semaphore_create_info();

  VK_ASSERT(vkCreateSemaphore(
      this->device,
      &semaphore_create_info,
      nullptr,
      &this->present_semaphore));

  VK_ASSERT(vkCreateSemaphore(
      this->device,
      &semaphore_create_info,
      nullptr,
      &this->render_semaphore));
}

void Render::VulkanEngine::init_pipelines() {
  VkShaderModule mono_frag;
  VkShaderModule mono_vert;
  VkShaderModule rainbow_frag;
  VkShaderModule rainbow_vert;
  VkShaderModule mesh_vert;

  Err err = Shader::load_shader_module(
      "shaders/color_frag.spv",
      this->device,
      &rainbow_frag);
  if (err.is_error) {
    io::error(err.msg);
  }

  err = Shader::load_shader_module(
      "shaders/color_vert.spv",
      this->device,
      &rainbow_vert);
  if (err.is_error) {
    io::error(err.msg);
  }

  err = Shader::load_shader_module(
      "shaders/tri_frag.spv",
      this->device,
      &mono_frag);
  if (err.is_error) {
    io::error(err.msg);
  }

  err = Shader::load_shader_module(
      "shaders/tri_vert.spv",
      this->device,
      &mono_vert);
  if (err.is_error) {
    io::error(err.msg);
  }

  err = Shader::load_shader_module(
      "shaders/mesh_vert.spv",
      this->device,
      &mesh_vert);
  if (err.is_error) {
    io::error(err.msg);
  }

  VkViewport viewport = {};
  viewport.x = 0.0;
  viewport.y = 0.0;
  viewport.width = (float)this->dimensions.width;
  viewport.height = (float)this->dimensions.height;
  viewport.minDepth = 0.0f;
  viewport.maxDepth = 1.0f;

  VkRect2D scissor = {};
  scissor.offset = {0, 0};
  scissor.extent.width = this->dimensions.width;
  scissor.extent.height = this->dimensions.height;

  VkPipelineLayoutCreateInfo pipeline_layout_info =
      VkInit::pipeline_layout_create_info();
  VK_ASSERT(vkCreatePipelineLayout(
      this->device,
      &pipeline_layout_info,
      nullptr,
      &this->pipeline_layout));

  PipelineBuilder builder;

  this->mono_pipeline =
      builder
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_VERTEX_BIT,
              mono_vert))
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_FRAGMENT_BIT,
              mono_frag))
          .with_vertex_input(VkInit::vertex_input_state_create_info())
          .with_input_assembly(VkInit::input_assembly_state_create_info(
              VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST))
          .with_viewport(viewport)
          .with_scissor(scissor)
          .with_rasterizer(
              VkInit::rasterization_state_create_info(VK_POLYGON_MODE_FILL))
          .with_color_blend_attachment(VkInit::color_blend_attachment_state())
          .with_multisample(VkInit::multisample_state_create_info())
          .with_pipeline_layout(this->pipeline_layout)
          .build(this->device, this->render_pass);

  this->rainbow_pipeline =
      builder.clear_shader_stages()
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_VERTEX_BIT,
              rainbow_vert))
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_FRAGMENT_BIT,
              rainbow_frag))
          .build(this->device, this->render_pass);

  VertexInputDescription vertex_input = Vertex::get_description();
  io::info("num_attributes: {}", vertex_input.attributes.size());
  this->mesh_pipeline =
      builder.clear_shader_stages()
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_VERTEX_BIT,
              mesh_vert))
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_FRAGMENT_BIT,
              rainbow_frag))
          .with_vertex_input(
              VkInit::vertex_input_state_create_info(&vertex_input))
          .build(this->device, this->render_pass);

  vkDestroyShaderModule(device, mono_frag, nullptr);
  vkDestroyShaderModule(device, mono_vert, nullptr);
  vkDestroyShaderModule(device, rainbow_frag, nullptr);
  vkDestroyShaderModule(device, rainbow_vert, nullptr);
  vkDestroyShaderModule(device, mesh_vert, nullptr);
}

void Render::VulkanEngine::init_allocator() {
  VmaAllocatorCreateInfo allocator_info = {};
  allocator_info.physicalDevice = this->gpu;
  allocator_info.device = this->device;
  allocator_info.instance = this->instance;
  vmaCreateAllocator(&allocator_info, &this->allocator);
}

void Render::VulkanEngine::init_meshes() {
  this->triangle_mesh.vertices.resize(3);

  this->triangle_mesh.vertices[0].position = {1.0f, 1.0f, 0.0f};
  this->triangle_mesh.vertices[1].position = {-1.0f, 1.0f, 0.0f};
  this->triangle_mesh.vertices[2].position = {0.0f, -1.0f, 0.0f};

  this->triangle_mesh.vertices[0].color = {0.0f, 1.0f, 0.0f};
  this->triangle_mesh.vertices[1].color = {0.0f, 1.0f, 0.0f};
  this->triangle_mesh.vertices[2].color = {0.0f, 1.0f, 0.0f};

  this->upload_mesh(this->triangle_mesh);
}

void Render::VulkanEngine::init_imgui() {
  // IMGUI_CHECKVERSION();
  // ImGui::CreateContext();
  // ImGuiIO &io = ImGui::GetIO();
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  // io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
  //
  // ImGui::StyleColorsDark();
  //
  // ImGui_ImplGlfw_InitForVulkan(this->window.raw_window_handle(), true);
  // ImGui_ImplVulkan_InitInfo init_info = {};
  // init_info.Instance = this->instance;
  // init_info.PhysicalDevice = this->gpu;
  // init_info.Device = this->device;
  // init_info.QueueFamily = this->graphics_queue_family;
  // init_info.Queue = this->graphics_queue;
  // init_info.PipelineCache = g_PipelineCache;
  // init_info.DescriptorPool = g_DescriptorPool;
  // init_info.RenderPass = wd->RenderPass;
  // init_info.Subpass = 0;
  // init_info.MinImageCount = g_MinImageCount;
  // init_info.ImageCount = wd->ImageCount;
  // init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  // init_info.Allocator = g_Allocator;
  // init_info.CheckVkResultFn = check_vk_result;
  // ImGui_ImplVulkan_Init(&init_info);
}

void Render::VulkanEngine::upload_mesh(Mesh &mesh) {
  VkBufferCreateInfo buffer_info = {};
  buffer_info.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  buffer_info.pNext = nullptr;
  buffer_info.size = mesh.vertices.size() * sizeof(Vertex);
  buffer_info.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;

  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

  VK_ASSERT(vmaCreateBuffer(
      this->allocator,
      &buffer_info,
      &alloc_info,
      &mesh.vertex_buffer.buffer,
      &mesh.vertex_buffer.allocation,
      nullptr));

  void *data;
  vmaMapMemory(this->allocator, mesh.vertex_buffer.allocation, &data);

  memcpy(data, mesh.vertices.data(), mesh.size());

  vmaUnmapMemory(allocator, mesh.vertex_buffer.allocation);
}
