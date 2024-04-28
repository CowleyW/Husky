#include "vk_engine.h"

#include "core/perf.h"
#include "ecs/components.h"
#include "entt/entity/fwd.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "io/assets.h"
#include "io/files.h"
#include "io/logging.h"
#include "render/bounding_boxes.h"
#include "render/callback_handler.h"
#include "render/pipeline_builder.h"
#include "render/shader.h"
#include "render/tri_mesh.h"
#include "render/vertex.h"
#include "render/vk_init.h"
#include "render/vk_types.h"
#include "util/serialize.h"

#include "VkBootstrap.h"
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <entt/entt.hpp>
#include <iterator>
#include <tracy/Tracy.hpp>
#include <vector>
#include <vulkan/vulkan_core.h>

#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

Render::VulkanEngine::~VulkanEngine() {
  // Make sure the GPU is not doing anything before we do any cleanup
  vkDeviceWaitIdle(this->device);

  for (auto &[key, value] : this->textures) {
    vmaDestroyImage(this->allocator, value.image.image, value.image.allocation);
    vkDestroyImageView(this->device, value.image_view, nullptr);
  }

  while (!cleanup_fns.empty()) {
    auto &destructor = cleanup_fns.top();
    destructor();

    cleanup_fns.pop();
  }
}

Render::VulkanEngine::VulkanEngine(
    Dimensions dimensions,
    CallbackHandler *handler)
    : imgui_fns(),
      cleanup_fns() {
  this->dimensions = dimensions;
  this->window.init(dimensions);
  this->window.register_callbacks(handler);

  this->init_vulkan();
  this->init_allocator();
  this->init_buffers();
  this->init_compute();
  this->init_sampler();
  this->init_descriptors();
  this->init_frames();
  this->init_swapchain();
  this->init_default_renderpass();
  this->init_framebuffers();
  this->init_pipelines();
  this->init_imgui();
}

void Render::VulkanEngine::destroy_swapchain() {
  vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);
  for (uint32_t i = 0; i < this->swapchain_image_views.size(); i += 1) {
    vkDestroyFramebuffer(this->device, this->frame_buffers[i], nullptr);
    vkDestroyImageView(this->device, this->swapchain_image_views[i], nullptr);
  }
  vkDestroyImageView(this->device, this->depth_image_view, nullptr);

  vmaDestroyImage(
      this->allocator,
      this->depth_image.image,
      this->depth_image.allocation);
}

uint32_t Render::VulkanEngine::prepare_frame(Frame &frame) {
  ZoneScoped;

  uint32_t next_image_index;

  VK_ASSERT(vkAcquireNextImageKHR(
      this->device,
      this->swapchain,
      1000000000,
      frame.present_semaphore,
      nullptr,
      &next_image_index));

  return next_image_index;
}

std::pair<CullData, CameraData> Render::VulkanEngine::get_camera_data(
    entt::registry &registry,
    uint32_t total_objects) {
  ZoneScoped;

  auto view = registry.view<Camera, Transform>();
  entt::entity camera_entity;
  for (auto entity : view) {
    camera_entity = entity;
    break;
  }
  Camera &camera = view.get<Camera>(camera_entity);
  Transform &t = view.get<Transform>(camera_entity);

  glm::mat4 viewproj = camera.calc_viewproj(t.position, this->dimensions);
  CameraData camera_data = {viewproj};
  CullData cull_data(viewproj, total_objects);

  return {cull_data, camera_data};
}

void Render::VulkanEngine::prepare_imgui_data() {
  ZoneScoped;

  ImGui_ImplVulkan_NewFrame();
  ImGui_ImplGlfw_NewFrame();
  ImGui::NewFrame();

  ImGui::Begin("Draw Stats");
  ImGui::Text("Instances: %d", this->draw_stats.draw_count);
  ImGui::Text("Indices Pre-cull: %d", this->draw_stats.precull_indices);
  ImGui::Text("Indices Post-cull: %d", this->draw_stats.postcull_indices);
  ImGui::Text("AABB Vertices: %d", this->draw_stats.aabb_vertices);
  ImGui::End();

  for (auto fn : this->imgui_fns) {
    fn();
  }

  this->imgui_fns.clear();

  ImGui::Render();
  ImDrawData *main_draw_data = ImGui::GetDrawData();
}

void Render::VulkanEngine::render(entt::registry &registry) {
  ZoneScoped;
  Frame &frame = this->next_frame();

  frame.await_compute(this->device);
  frame.await_render(this->device);

  uint32_t total_objects = 0;
  std::vector<Batch> batches =
      frame.copy_renderable_objects(registry, total_objects, this->allocator);

  auto pair = this->get_camera_data(registry, total_objects);
  CullData cull = pair.first;
  CameraData camera = pair.second;

  frame.copy_cull_data(this->allocator, cull);
  frame.copy_camera_data(this->allocator, camera);

  frame.prepare_indirect_buffer(batches, this->allocator);
  frame.prepare_compute_commands(this->compute);
  frame.submit_compute(this->compute, total_objects);

  uint32_t next_image_index = this->prepare_frame(frame);

  frame.begin_render_pass(
      this->render_pass,
      this->frame_buffers[next_image_index],
      this->dimensions);
  frame.bind_pipeline(this->mesh_pipeline, this->dimensions);

  this->scene_data.ambient_color = {1.0f, 1.0f, 1.0f, 1.0f};

  void *scene_data;
  vmaMapMemory(
      this->allocator,
      this->scene_data_buffer.allocation,
      &scene_data);

  uint32_t buffer_offset =
      AllocatedBuffer::padding_size(sizeof(SceneData), this->gpu_properties);
  uint32_t frame_index =
      this->frame_number % Render::VulkanEngine::FRAMES_IN_FLIGHT;
  std::memcpy(
      (char *)scene_data + (buffer_offset * frame_index),
      &this->scene_data,
      sizeof(SceneData));
  vmaUnmapMemory(this->allocator, scene_data_buffer.allocation);

  frame.bind_descriptor_sets(this->mesh_pipeline_layout, buffer_offset);
  frame.prepare_graphics_buffers(
      this->vertex_buffer,
      this->index_buffer,
      batches.size());

  frame.bind_pipeline(this->aabb_pipeline, this->dimensions);
  vkCmdBindDescriptorSets(
      frame.main_command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->aabb_pipeline_layout,
      0,
      1,
      &frame.aabb_descriptor,
      0,
      nullptr);
  vkCmdDrawIndirect(
      frame.main_command_buffer,
      frame.aabb_draw_buffer.buffer,
      0,
      batches.size(),
      sizeof(VkDrawIndirectCommand));

  this->prepare_imgui_data();
  frame.submit_draw(this->swapchain, this->graphics_queue, next_image_index);

  void *data;
  vmaMapMemory(this->allocator, frame.draw_stats_buffer.allocation, &data);
  DrawStats draw_stats = *(DrawStats *)data;
  vmaUnmapMemory(this->allocator, frame.draw_stats_buffer.allocation);

  if (draw_stats.precull_indices != 0) {
    this->draw_stats = draw_stats;
  }
  this->frame_number += 1;
}

void Render::VulkanEngine::resize(Dimensions dimensions) {
  vkDeviceWaitIdle(this->device);

  this->destroy_swapchain();

  this->dimensions = dimensions;

  this->init_swapchain();
  this->init_framebuffers();
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
                      .require_api_version(1, 3, 0)
                      .build();

  vkb::Instance vkb_inst = inst_ret.value();

  this->instance = vkb_inst.instance;
  this->debug_messenger = vkb_inst.debug_messenger;

  this->surface = this->window.create_surface(this->instance);

  VkPhysicalDeviceDescriptorIndexingFeatures indexing = {};
  indexing.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES;
  indexing.pNext = nullptr;
  indexing.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
  indexing.runtimeDescriptorArray = VK_TRUE;
  indexing.descriptorBindingVariableDescriptorCount = VK_TRUE;
  indexing.descriptorBindingPartiallyBound = VK_TRUE;
  indexing.descriptorBindingSampledImageUpdateAfterBind = VK_TRUE;

  VkPhysicalDeviceFeatures features = {};
  features.multiDrawIndirect = VK_TRUE;
  features.fillModeNonSolid = VK_TRUE;

  vkb::PhysicalDeviceSelector selector(vkb_inst);
  vkb::PhysicalDevice gpu =
      selector.set_minimum_version(1, 1)
          .set_surface(this->surface)
          .add_required_extension(VK_EXT_DESCRIPTOR_INDEXING_EXTENSION_NAME)
          .add_required_extension_features(indexing)
          .set_required_features(features)
          .select()
          .value();

  vkb::DeviceBuilder device_builder(gpu);
  VkPhysicalDeviceShaderDrawParameterFeatures shader_draw = {};
  shader_draw.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;
  shader_draw.pNext = nullptr;
  shader_draw.shaderDrawParameters = VK_TRUE;

  vkb::Device device = device_builder.add_pNext(&shader_draw).build().value();

  this->gpu = gpu.physical_device;
  this->device = device.device;
  this->gpu_properties = device.physical_device.properties;

  this->graphics_queue = device.get_queue(vkb::QueueType::graphics).value();
  this->graphics_queue_family =
      device.get_queue_index(vkb::QueueType::graphics).value();

  this->compute.queue = device.get_queue(vkb::QueueType::compute).value();
  this->compute.queue_family =
      device.get_queue_index(vkb::QueueType::compute).value();

  this->cleanup_fns.push(
      [this]() { vkDestroyInstance(this->instance, nullptr); });

  this->cleanup_fns.push([this]() {
    vkDestroyDevice(this->device, nullptr);
    vkDestroySurfaceKHR(this->instance, this->surface, nullptr);
    vkb::destroy_debug_utils_messenger(this->instance, this->debug_messenger);
  });
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
  this->swapchain_extent = vkb_swapchain.extent;
  this->swapchain_image_views = vkb_swapchain.get_image_views().value();
  this->swapchain_format = vkb_swapchain.image_format;

  VkExtent3D depth_extent = {
      this->dimensions.width,
      this->dimensions.height,
      1};

  this->depth_format = VK_FORMAT_D32_SFLOAT;

  VkImageCreateInfo depth_create_info = VkInit::image_create_info(
      this->depth_format,
      VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
      depth_extent);

  VmaAllocationCreateInfo depth_alloc_info = {};
  depth_alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;
  depth_alloc_info.requiredFlags =
      VkMemoryPropertyFlags(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  vmaCreateImage(
      this->allocator,
      &depth_create_info,
      &depth_alloc_info,
      &this->depth_image.image,
      &this->depth_image.allocation,
      nullptr);

  VkImageViewCreateInfo depth_view_info = VkInit::imageview_create_info(
      this->depth_format,
      this->depth_image.image,
      VK_IMAGE_ASPECT_DEPTH_BIT);

  VK_ASSERT(vkCreateImageView(
      this->device,
      &depth_view_info,
      nullptr,
      &this->depth_image_view));

  this->cleanup_fns.push([this]() { this->destroy_swapchain(); });
}

void Render::VulkanEngine::init_default_renderpass() {
  VkAttachmentDescription color_attachment =
      VkInit::color_attachment(this->swapchain_format);
  VkAttachmentReference color_attachment_ref = VkInit::color_attachment_ref();

  VkAttachmentDescription depth_attachment =
      VkInit::depth_attachment(this->depth_format);
  VkAttachmentReference depth_attachment_ref = VkInit::depth_attachment_ref();

  VkSubpassDescription subpass = VkInit::subpass_description(
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      &color_attachment_ref,
      &depth_attachment_ref);

  VkSubpassDependency dependency = {};
  dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  dependency.dstSubpass = 0;
  dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.srcAccessMask = 0;
  dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
  dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

  VkSubpassDependency depth_dependency = {};
  depth_dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
  depth_dependency.dstSubpass = 0;
  depth_dependency.srcStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depth_dependency.srcAccessMask = 0;
  depth_dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT |
                                  VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
  depth_dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

  std::vector<VkAttachmentDescription> attachments = {
      color_attachment,
      depth_attachment};
  std::vector<VkSubpassDependency> dependencies = {
      dependency,
      depth_dependency};
  VkRenderPassCreateInfo render_pass_info =
      VkInit::render_pass_create_info(&attachments, &dependencies, &subpass);

  VK_ASSERT(vkCreateRenderPass(
      this->device,
      &render_pass_info,
      nullptr,
      &this->render_pass));

  this->cleanup_fns.push([this]() {
    vkDestroyRenderPass(this->device, this->render_pass, nullptr);
  });
}

void Render::VulkanEngine::init_descriptors() {
  std::vector<VkDescriptorPoolSize> sizes = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 50},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
  pool_info.maxSets = 1070;
  pool_info.poolSizeCount = sizes.size();
  pool_info.pPoolSizes = sizes.data();

  vkCreateDescriptorPool(
      this->device,
      &pool_info,
      nullptr,
      &this->descriptor_pool);

  VkDescriptorSetLayoutBinding camera_buffer_binding =
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          VK_SHADER_STAGE_VERTEX_BIT,
          0);

  VkDescriptorSetLayoutBinding scene_buffer_binding =
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
          VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT,
          1);

  VkDescriptorSetLayoutBinding bindings[2] = {
      camera_buffer_binding,
      scene_buffer_binding};

  VkDescriptorSetLayoutCreateInfo set0_info = {};
  set0_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set0_info.pNext = nullptr;
  set0_info.flags = 0;
  set0_info.bindingCount = 2;
  set0_info.pBindings = bindings;

  VK_ASSERT(vkCreateDescriptorSetLayout(
      this->device,
      &set0_info,
      nullptr,
      &this->global_descriptor_layout));

  VkDescriptorSetLayoutBinding compute_instance_buffer_binding =
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_VERTEX_BIT,
          0);

  VkDescriptorSetLayoutCreateInfo set1_info = {};
  set1_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set1_info.pNext = nullptr;
  set1_info.flags = 0;
  set1_info.bindingCount = 1;
  set1_info.pBindings = &compute_instance_buffer_binding;

  vkCreateDescriptorSetLayout(
      this->device,
      &set1_info,
      nullptr,
      &this->object_descriptor_layout);

  VkDescriptorSetLayoutBinding texture_binding =
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
          VK_SHADER_STAGE_FRAGMENT_BIT,
          0,
          128);

  VkDescriptorBindingFlags flags =
      VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT |
      VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT |
      VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT;

  VkDescriptorSetLayoutBindingFlagsCreateInfo layout_binding_flags = {};
  layout_binding_flags.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO;
  layout_binding_flags.bindingCount = 1;
  layout_binding_flags.pBindingFlags = &flags;

  VkDescriptorSetLayoutCreateInfo texture_info = {};
  texture_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  texture_info.pNext = &layout_binding_flags;
  texture_info.flags =
      VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
  texture_info.bindingCount = 1;
  texture_info.pBindings = &texture_binding;

  vkCreateDescriptorSetLayout(
      this->device,
      &texture_info,
      nullptr,
      &this->texture_descriptor_layout);

  std::vector<VkDescriptorSetLayoutBinding> aabb_bindings = {
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          VK_SHADER_STAGE_VERTEX_BIT,
          0),
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_VERTEX_BIT,
          1),
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_VERTEX_BIT,
          2)};

  VkDescriptorSetLayoutCreateInfo set_layout_info = {};
  set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set_layout_info.pNext = nullptr;
  set_layout_info.flags = 0;
  set_layout_info.bindingCount = aabb_bindings.size();
  set_layout_info.pBindings = aabb_bindings.data();

  VK_ASSERT(vkCreateDescriptorSetLayout(
      this->device,
      &set_layout_info,
      nullptr,
      &this->aabb_descriptor_layout));

  this->cleanup_fns.push([this]() {
    vkDestroyDescriptorSetLayout(
        this->device,
        this->global_descriptor_layout,
        nullptr);
    vkDestroyDescriptorSetLayout(
        this->device,
        this->object_descriptor_layout,
        nullptr);
    vkDestroyDescriptorSetLayout(
        this->device,
        this->texture_descriptor_layout,
        nullptr);
    vkDestroyDescriptorSetLayout(
        this->device,
        this->aabb_descriptor_layout,
        nullptr);
    vkDestroyDescriptorPool(this->device, this->descriptor_pool, nullptr);
  });
}

void Render::VulkanEngine::init_frames() {
  VkCommandPoolCreateInfo graphics_pool_info = VkInit::command_pool_create_info(
      this->graphics_queue_family,
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  VkCommandPoolCreateInfo compute_pool_info = VkInit::command_pool_create_info(
      this->compute.queue_family,
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  VkFenceCreateInfo fence_create_info = VkInit::fence_create_info();
  VkSemaphoreCreateInfo semaphore_create_info = VkInit::semaphore_create_info();

  VkDescriptorSetAllocateInfo global_set_alloc =
      VkInit::descriptor_set_allocate_info(
          this->descriptor_pool,
          this->global_descriptor_layout);

  VkDescriptorSetAllocateInfo object_set_alloc =
      VkInit::descriptor_set_allocate_info(
          this->descriptor_pool,
          this->object_descriptor_layout);

  uint32_t max_descriptors = 128;

  VkDescriptorSetVariableDescriptorCountAllocateInfo variable_alloc_info = {};
  variable_alloc_info.sType =
      VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO;
  variable_alloc_info.descriptorSetCount = 1;
  variable_alloc_info.pDescriptorCounts = &max_descriptors;

  VkDescriptorSetAllocateInfo texture_set_alloc =
      VkInit::descriptor_set_allocate_info(
          this->descriptor_pool,
          this->texture_descriptor_layout);
  texture_set_alloc.pNext = &variable_alloc_info;

  VkDescriptorSetAllocateInfo compute_set_alloc =
      VkInit::descriptor_set_allocate_info(
          this->descriptor_pool,
          compute.descriptor_layout);

  VkDescriptorSetAllocateInfo aabb_descriptor_alloc =
      VkInit::descriptor_set_allocate_info(
          this->descriptor_pool,
          this->aabb_descriptor_layout);

  for (auto &frame : this->frames) {
    VK_ASSERT(vkCreateFence(
        this->device,
        &fence_create_info,
        nullptr,
        &frame.render_fence));
    VK_ASSERT(vkCreateFence(
        this->device,
        &fence_create_info,
        nullptr,
        &frame.compute_fence));

    VK_ASSERT(vkCreateSemaphore(
        this->device,
        &semaphore_create_info,
        nullptr,
        &frame.present_semaphore));

    VK_ASSERT(vkCreateSemaphore(
        this->device,
        &semaphore_create_info,
        nullptr,
        &frame.render_semaphore));

    VK_ASSERT(vkCreateSemaphore(
        this->device,
        &semaphore_create_info,
        nullptr,
        &frame.compute_semaphore));

    VK_ASSERT(vkCreateCommandPool(
        this->device,
        &graphics_pool_info,
        nullptr,
        &frame.graphics_command_pool));

    VK_ASSERT(vkCreateCommandPool(
        this->device,
        &compute_pool_info,
        nullptr,
        &frame.compute_command_pool));

    VkCommandBufferAllocateInfo graphics_alloc_info =
        VkInit::command_buffer_allocate_info(
            frame.graphics_command_pool,
            1,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkCommandBufferAllocateInfo compute_alloc_info =
        VkInit::command_buffer_allocate_info(
            frame.compute_command_pool,
            1,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VK_ASSERT(vkAllocateCommandBuffers(
        this->device,
        &graphics_alloc_info,
        &frame.main_command_buffer));

    VK_ASSERT(vkAllocateCommandBuffers(
        this->device,
        &compute_alloc_info,
        &frame.compute_command_buffer));

    frame.camera_buffer = VkInit::buffer(
        this->allocator,
        sizeof(CameraData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    frame.cull_buffer = VkInit::buffer(
        this->allocator,
        sizeof(CullData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    frame.compute_instance_buffer = VkInit::buffer(
        this->allocator,
        // ssbo SIZE
        sizeof(ComputeInstanceData) * VulkanEngine::MAX_INSTANCES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    frame.vertex_instance_buffer = VkInit::buffer(
        this->allocator,
        sizeof(VertexInstanceData) * VulkanEngine::MAX_INSTANCES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE);
    frame.indirect_buffer = VkInit::buffer(
        this->allocator,
        sizeof(VkDrawIndexedIndirectCommand) * 1000,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    frame.draw_stats_buffer = VkInit::buffer(
        this->allocator,
        sizeof(DrawStats),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT |
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        VMA_MEMORY_USAGE_CPU_COPY);
    frame.aabb_draw_buffer = VkInit::buffer(
        this->allocator,
        sizeof(VkDrawIndirectCommand) * VulkanEngine::MAX_INSTANCES,
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
            VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);

    VK_ASSERT(vkAllocateDescriptorSets(
        this->device,
        &global_set_alloc,
        &frame.global_descriptor));

    VK_ASSERT(vkAllocateDescriptorSets(
        this->device,
        &object_set_alloc,
        &frame.object_descriptor));

    VK_ASSERT(vkAllocateDescriptorSets(
        this->device,
        &texture_set_alloc,
        &frame.texture_descriptor));

    VK_ASSERT(vkAllocateDescriptorSets(
        this->device,
        &compute_set_alloc,
        &frame.compute_descriptor));

    VK_ASSERT(vkAllocateDescriptorSets(
        this->device,
        &aabb_descriptor_alloc,
        &frame.aabb_descriptor));

    VkDescriptorBufferInfo camera_info = {};
    camera_info.buffer = frame.camera_buffer.buffer;
    camera_info.offset = 0;
    camera_info.range = sizeof(CameraData);

    VkDescriptorBufferInfo cull_info = {};
    cull_info.buffer = frame.cull_buffer.buffer;
    cull_info.offset = 0;
    cull_info.range = frame.cull_buffer.range;

    VkDescriptorBufferInfo scene_info = {};
    scene_info.buffer = this->scene_data_buffer.buffer;
    scene_info.offset = 0;
    scene_info.range = sizeof(SceneData);

    VkDescriptorBufferInfo in_instance_info = {};
    in_instance_info.buffer = frame.compute_instance_buffer.buffer;
    in_instance_info.offset = 0;
    in_instance_info.range =
        sizeof(ComputeInstanceData) * VulkanEngine::MAX_INSTANCES;

    VkDescriptorBufferInfo out_instance_info = {};
    out_instance_info.buffer = frame.vertex_instance_buffer.buffer;
    out_instance_info.offset = 0;
    out_instance_info.range =
        sizeof(VertexInstanceData) * VulkanEngine::MAX_INSTANCES;

    VkDescriptorBufferInfo indirect_info =
        frame.indirect_buffer.descriptor_info();

    VkDescriptorBufferInfo draw_stats_info = {};
    draw_stats_info.buffer = frame.draw_stats_buffer.buffer;
    draw_stats_info.offset = 0;
    draw_stats_info.range = sizeof(DrawStats);

    VkDescriptorBufferInfo mesh_buffer_info = {};
    mesh_buffer_info.buffer = this->mesh_data_buffer.buffer;
    mesh_buffer_info.offset = 0;
    mesh_buffer_info.range = this->mesh_data_buffer.range;

    VkDescriptorBufferInfo aabb_draw_info = {};
    aabb_draw_info.buffer = frame.aabb_draw_buffer.buffer;
    aabb_draw_info.offset = 0;
    aabb_draw_info.range = sizeof(VkDrawIndirectCommand) * 1000;

    std::vector<VkWriteDescriptorSet> write_sets = {
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            frame.global_descriptor,
            &camera_info,
            0),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
            frame.global_descriptor,
            &scene_info,
            1),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frame.object_descriptor,
            &out_instance_info,
            0),

        // Descriptor sets for the compute shader
        // 0. is for the CPU-supplied transform vectors
        // 1. is for the outputted transformation matrices
        // 2. is for the indirect commands buffer
        // 3. is for the draw stats buffer
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frame.compute_descriptor,
            &in_instance_info,
            0),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frame.compute_descriptor,
            &out_instance_info,
            1),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frame.compute_descriptor,
            &indirect_info,
            2),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            frame.compute_descriptor,
            &cull_info,
            3),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            frame.compute_descriptor,
            &camera_info,
            4),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frame.compute_descriptor,
            &draw_stats_info,
            5),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frame.compute_descriptor,
            &mesh_buffer_info,
            6),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frame.compute_descriptor,
            &aabb_draw_info,
            7),

        // Descriptor sets for the aabb shader
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
            frame.aabb_descriptor,
            &camera_info,
            0),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frame.aabb_descriptor,
            &mesh_buffer_info,
            1),
        VkInit::write_descriptor_set(
            VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
            frame.aabb_descriptor,
            &out_instance_info,
            2)};

    vkUpdateDescriptorSets(
        this->device,
        write_sets.size(),
        write_sets.data(),
        0,
        nullptr);
  }

  this->cleanup_fns.push([this]() {
    for (auto &frame : this->frames) {
      frame.destroy(this->device, this->allocator);
    }
  });
}

void Render::VulkanEngine::init_framebuffers() {
  VkFramebufferCreateInfo frame_buffer_info =
      VkInit::frame_buffer_create_info(this->render_pass, this->dimensions);

  const uint32_t num_images = this->swapchain_images.size();
  this->frame_buffers = std::vector<VkFramebuffer>(num_images);

  for (uint32_t i = 0; i < num_images; i += 1) {
    VkImageView attachments[] = {
        this->swapchain_image_views[i],
        this->depth_image_view};
    frame_buffer_info.attachmentCount = 2;
    frame_buffer_info.pAttachments = attachments;

    VK_ASSERT(vkCreateFramebuffer(
        this->device,
        &frame_buffer_info,
        nullptr,
        &this->frame_buffers[i]));
  }
}

void Render::VulkanEngine::init_pipelines() {
  VkShaderModule frag;
  VkShaderModule vert;

  Err err = Shader::load_shader_module(
      "shaders/standard.frag.spv",
      this->device,
      &frag);
  if (err.is_error) {
    io::error(err.msg);
  }

  err = Shader::load_shader_module(
      "shaders/standard.vert.spv",
      this->device,
      &vert);
  if (err.is_error) {
    io::error(err.msg);
  }

  VkDynamicState dynamic_state[2] = {
      VK_DYNAMIC_STATE_SCISSOR,
      VK_DYNAMIC_STATE_VIEWPORT};

  PipelineBuilder builder;

  std::vector<VkPushConstantRange> push_constant(1);
  push_constant[0].offset = 0;
  push_constant[0].size = sizeof(MeshPushConstant);
  push_constant[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  std::vector<VkDescriptorSetLayout> descriptor_layouts;
  descriptor_layouts.push_back(this->global_descriptor_layout);
  descriptor_layouts.push_back(this->object_descriptor_layout);
  descriptor_layouts.push_back(this->texture_descriptor_layout);

  VkPipelineLayoutCreateInfo mesh_layout_info =
      VkInit::pipeline_layout_create_info(&push_constant, &descriptor_layouts);
  VK_ASSERT(vkCreatePipelineLayout(
      this->device,
      &mesh_layout_info,
      nullptr,
      &this->mesh_pipeline_layout));

  VertexInputDescription vertex_input = Vertex::get_description();
  this->mesh_pipeline =
      builder.clear_shader_stages()
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_VERTEX_BIT,
              vert))
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_FRAGMENT_BIT,
              frag))
          .with_vertex_input(
              VkInit::vertex_input_state_create_info(&vertex_input))
          .with_pipeline_layout(this->mesh_pipeline_layout)
          .with_input_assembly(VkInit::input_assembly_state_create_info(
              VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST))
          .with_dynamic_state(dynamic_state, 2)
          .with_rasterizer(
              VkInit::rasterization_state_create_info(VK_POLYGON_MODE_FILL))
          .with_color_blend_attachment(VkInit::color_blend_attachment_state())
          .with_multisample(VkInit::multisample_state_create_info())
          .with_depth_stencil(VkInit::depth_stencil_create_info(
              true,
              true,
              VK_COMPARE_OP_LESS_OR_EQUAL))
          .build(this->device, this->render_pass);

  vkDestroyShaderModule(device, frag, nullptr);
  vkDestroyShaderModule(device, vert, nullptr);

  this->init_aabb_pipeline();

  this->cleanup_fns.push([this]() {
    vkDestroyPipeline(this->device, this->mesh_pipeline, nullptr);
    vkDestroyPipelineLayout(this->device, this->mesh_pipeline_layout, nullptr);
  });
}

void Render::VulkanEngine::init_allocator() {
  VmaAllocatorCreateInfo allocator_info = {};
  allocator_info.physicalDevice = this->gpu;
  allocator_info.device = this->device;
  allocator_info.instance = this->instance;
  vmaCreateAllocator(&allocator_info, &this->allocator);

  this->cleanup_fns.push([this]() { vmaDestroyAllocator(this->allocator); });
}

void Render::VulkanEngine::init_buffers() {
  this->mesh_data_buffer = VkInit::buffer(
      this->allocator,
      sizeof(AABB) * VulkanEngine::MAX_INSTANCES,
      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);
  this->vertex_buffer = VkInit::buffer(
      this->allocator,
      1024 * 1024 * 50,
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  this->index_buffer = VkInit::buffer(
      this->allocator,
      1024 * 1024 * 50,
      VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
      VMA_MEMORY_USAGE_GPU_ONLY);

  this->indirect_commands_buffer = VkInit::buffer(
      this->allocator,
      5000 * sizeof(VkDrawIndexedIndirectCommand),
      VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_STORAGE_BUFFER_BIT |
          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  const uint32_t scene_data_size =
      Render::VulkanEngine::FRAMES_IN_FLIGHT *
      AllocatedBuffer::padding_size(sizeof(SceneData), this->gpu_properties);
  this->scene_data_buffer = VkInit::buffer(
      this->allocator,
      scene_data_size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  this->cleanup_fns.push([this]() {
    this->vertex_buffer.destroy(this->allocator);
    this->index_buffer.destroy(this->allocator);
    this->mesh_data_buffer.destroy(this->allocator);
    this->indirect_commands_buffer.destroy(this->allocator);
    this->scene_data_buffer.destroy(this->allocator);
  });
}

void Render::VulkanEngine::init_aabb_pipeline() {
  std::vector<VkDescriptorSetLayout> descriptor_layouts = {
      this->aabb_descriptor_layout};
  VkPipelineLayoutCreateInfo aabb_layout_info =
      VkInit::pipeline_layout_create_info(nullptr, &descriptor_layouts);
  VK_ASSERT(vkCreatePipelineLayout(
      this->device,
      &aabb_layout_info,
      nullptr,
      &this->aabb_pipeline_layout));

  VkShaderModule frag;
  VkShaderModule vert;

  Err err =
      Shader::load_shader_module("shaders/aabb.frag.spv", this->device, &frag);
  if (err.is_error) {
    io::error(err.msg);
  }

  err =
      Shader::load_shader_module("shaders/aabb.vert.spv", this->device, &vert);
  if (err.is_error) {
    io::error(err.msg);
  }

  VkDynamicState dynamic_state[2] = {
      VK_DYNAMIC_STATE_SCISSOR,
      VK_DYNAMIC_STATE_VIEWPORT};

  VertexInputDescription input = {};
  PipelineBuilder builder;
  this->aabb_pipeline =
      builder.clear_shader_stages()
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_VERTEX_BIT,
              vert))
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_FRAGMENT_BIT,
              frag))
          .with_vertex_input(VkInit::vertex_input_state_create_info(&input))
          .with_input_assembly(VkInit::input_assembly_state_create_info(
              VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST))
          .with_dynamic_state(dynamic_state, 2)
          .with_rasterizer(VkInit::rasterization_state_create_info(
              VK_POLYGON_MODE_LINE,
              VK_CULL_MODE_NONE))
          .with_color_blend_attachment(VkInit::color_blend_attachment_state())
          .with_multisample(VkInit::multisample_state_create_info())
          .with_pipeline_layout(this->aabb_pipeline_layout)
          .with_depth_stencil(VkInit::depth_stencil_create_info(
              true,
              true,
              VK_COMPARE_OP_LESS_OR_EQUAL))
          .build(this->device, this->render_pass);

  vkDestroyShaderModule(this->device, vert, nullptr);
  vkDestroyShaderModule(this->device, frag, nullptr);

  this->cleanup_fns.push([this]() {
    vkDestroyPipeline(this->device, this->aabb_pipeline, nullptr);
    vkDestroyPipelineLayout(this->device, this->aabb_pipeline_layout, nullptr);
  });
}

void Render::VulkanEngine::init_compute() {
  std::vector<VkDescriptorSetLayoutBinding> bindings = {
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_COMPUTE_BIT,
          0),
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_COMPUTE_BIT,
          1),
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_COMPUTE_BIT,
          2),
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          VK_SHADER_STAGE_COMPUTE_BIT,
          3),
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
          VK_SHADER_STAGE_COMPUTE_BIT,
          4),
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_COMPUTE_BIT,
          5),
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_COMPUTE_BIT,
          6),
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_COMPUTE_BIT,
          7)};

  VkDescriptorSetLayoutCreateInfo set_layout_info = {};
  set_layout_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set_layout_info.pNext = nullptr;
  set_layout_info.flags = 0;
  set_layout_info.bindingCount = bindings.size();
  set_layout_info.pBindings = bindings.data();

  VK_ASSERT(vkCreateDescriptorSetLayout(
      this->device,
      &set_layout_info,
      nullptr,
      &this->compute.descriptor_layout));

  std::vector<VkDescriptorSetLayout> descriptor_layouts = {
      compute.descriptor_layout};
  VkPipelineLayoutCreateInfo pipeline_layout_info =
      VkInit::pipeline_layout_create_info(nullptr, &descriptor_layouts);

  VK_ASSERT(vkCreatePipelineLayout(
      this->device,
      &pipeline_layout_info,
      nullptr,
      &compute.pipeline_layout));

  VkShaderModule comp;
  Err err =
      Shader::load_shader_module("shaders/cull.comp.spv", this->device, &comp);
  if (err.is_error) {
    io::error(err.msg);
  }

  VkPipelineShaderStageCreateInfo shader_stage =
      VkInit::pipeline_shader_stage_create_info(
          VK_SHADER_STAGE_COMPUTE_BIT,
          comp);

  VkComputePipelineCreateInfo pipeline_info = {};
  pipeline_info.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
  pipeline_info.layout = compute.pipeline_layout;
  pipeline_info.stage = shader_stage;

  VK_ASSERT(vkCreateComputePipelines(
      this->device,
      VK_NULL_HANDLE,
      1,
      &pipeline_info,
      nullptr,
      &compute.pipeline));

  vkDestroyShaderModule(this->device, comp, nullptr);

  this->cleanup_fns.push([this]() {
    vkDestroyPipeline(this->device, this->compute.pipeline, nullptr);
    vkDestroyPipelineLayout(
        this->device,
        this->compute.pipeline_layout,
        nullptr);
    vkDestroyDescriptorSetLayout(
        this->device,
        this->compute.descriptor_layout,
        nullptr);
  });
}

void Render::VulkanEngine::init_sampler() {
  VkSamplerCreateInfo sampler_info =
      VkInit::sampler_create_info(VK_FILTER_NEAREST);

  VK_ASSERT(
      vkCreateSampler(this->device, &sampler_info, nullptr, &this->sampler));

  this->cleanup_fns.push(
      [this]() { vkDestroySampler(this->device, this->sampler, nullptr); });
}

void Render::VulkanEngine::init_imgui() {
  VkDescriptorPoolSize pool_sizes[] = {
      {VK_DESCRIPTOR_TYPE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000},
      {VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000},
      {VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
  pool_info.maxSets = 1000 * IM_ARRAYSIZE(pool_sizes);
  pool_info.poolSizeCount = (uint32_t)IM_ARRAYSIZE(pool_sizes);
  pool_info.pPoolSizes = pool_sizes;

  VK_ASSERT(vkCreateDescriptorPool(
      this->device,
      &pool_info,
      nullptr,
      &this->imgui_descriptor_pool));

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForVulkan(this->window.raw_window_handle(), true);
  ImGui_ImplVulkan_InitInfo init_info = {};
  init_info.Instance = this->instance;
  init_info.PhysicalDevice = this->gpu;
  init_info.Device = this->device;
  init_info.QueueFamily = this->graphics_queue_family;
  init_info.Queue = this->graphics_queue;
  init_info.DescriptorPool = this->imgui_descriptor_pool;
  init_info.RenderPass = this->render_pass;
  init_info.Subpass = 0;
  init_info.MinImageCount = VulkanEngine::FRAMES_IN_FLIGHT;
  init_info.ImageCount = VulkanEngine::FRAMES_IN_FLIGHT;
  init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
  init_info.Allocator = nullptr;
  init_info.CheckVkResultFn = [](VkResult res) { VK_ASSERT(res); };
  ImGui_ImplVulkan_Init(&init_info);

  ImGui_ImplVulkan_CreateFontsTexture();

  this->cleanup_fns.push([this]() {
    ImGui_ImplVulkan_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    vkDestroyDescriptorPool(this->device, this->imgui_descriptor_pool, nullptr);
  });
}

void Render::VulkanEngine::upload_mesh(TriMeshHandle handle) {
  auto maybe = TriMesh::get(handle);
  if (maybe.is_error) {
    io::error(maybe.msg);
    return;
  }

  TriMesh *mesh = maybe.value;

  uint32_t vertex_buffer_size = mesh->vertices.size() * sizeof(Vertex);
  uint32_t index_buffer_size = mesh->indices.size() * sizeof(uint32_t);
  uint32_t aabb_size = sizeof(AABB);
  uint32_t buffer_size =
      std::max(std::max(vertex_buffer_size, index_buffer_size), aabb_size);
  mesh->first_vertex = this->vertex_buffer_offset / sizeof(Vertex);
  mesh->first_index = this->index_buffer_offset / sizeof(uint32_t);
  AllocatedBuffer staging_buffer = VkInit::buffer(
      this->allocator,
      buffer_size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_CPU_ONLY);

  // Upload the vertex data
  void *data;
  vmaMapMemory(this->allocator, staging_buffer.allocation, &data);

  memcpy(data, mesh->vertices.data(), vertex_buffer_size);

  vmaUnmapMemory(allocator, staging_buffer.allocation);

  this->submit_command([=](VkCommandBuffer cmd) {
    VkBufferCopy copy = {};
    copy.size = vertex_buffer_size;
    copy.srcOffset = 0;
    copy.dstOffset = this->vertex_buffer_offset;
    vkCmdCopyBuffer(
        cmd,
        staging_buffer.buffer,
        this->vertex_buffer.buffer,
        1,
        &copy);

    this->vertex_buffer_offset += vertex_buffer_size;

    io::debug("Vertex Buffer Offset: {}", this->vertex_buffer_offset);
    io::debug("Vertex Buffer Size: {}", vertex_buffer_size);
  });

  // Upload the index data
  vmaMapMemory(this->allocator, staging_buffer.allocation, &data);

  memcpy(data, mesh->indices.data(), index_buffer_size);

  vmaUnmapMemory(allocator, staging_buffer.allocation);

  this->submit_command([=](VkCommandBuffer cmd) {
    VkBufferCopy copy = {};
    copy.size = index_buffer_size;
    copy.srcOffset = 0;
    copy.dstOffset = this->index_buffer_offset;
    vkCmdCopyBuffer(
        cmd,
        staging_buffer.buffer,
        this->index_buffer.buffer,
        1,
        &copy);

    this->index_buffer_offset += index_buffer_size;
  });

  // Upload the bounding box data
  vmaMapMemory(this->allocator, staging_buffer.allocation, &data);

  AABB aabb = mesh->aabb();
  io::debug("AABB min: [{}, {}, {}]", aabb.min.x, aabb.min.y, aabb.min.z);
  io::debug("AABB max: [{}, {}, {}]", aabb.max.x, aabb.max.y, aabb.max.z);
  memcpy(data, &aabb, sizeof(AABB));

  vmaUnmapMemory(allocator, staging_buffer.allocation);

  this->submit_command([=](VkCommandBuffer cmd) {
    VkBufferCopy copy = {};
    copy.size = sizeof(AABB);
    copy.srcOffset = 0;
    copy.dstOffset = (uint32_t)handle * sizeof(AABB);
    vkCmdCopyBuffer(
        cmd,
        staging_buffer.buffer,
        this->mesh_data_buffer.buffer,
        1,
        &copy);
  });

  vmaDestroyBuffer(
      this->allocator,
      staging_buffer.buffer,
      staging_buffer.allocation);
}

void Render::VulkanEngine::upload_material(Material *material) {
  PERF_BEGIN(LoadTexture);
  auto res_image = this->load_texture_asset(material->material_name);
  PERF_END(LoadTexture);

  if (res_image.is_error) {
    io::error(res_image.msg);
  }

  Texture texture = {};
  texture.image = res_image.value;

  VkImageViewCreateInfo image_view_info = VkInit::imageview_create_info(
      VK_FORMAT_R8G8B8A8_SRGB,
      texture.image.image,
      VK_IMAGE_ASPECT_COLOR_BIT);
  VK_ASSERT(vkCreateImageView(
      this->device,
      &image_view_info,
      nullptr,
      &texture.image_view));

  this->textures[material->material_name] = texture;

  // VkDescriptorSetAllocateInfo alloc_info = {};
  // alloc_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
  // alloc_info.pNext = nullptr;
  // alloc_info.descriptorPool = this->descriptor_pool;
  // alloc_info.descriptorSetCount = 1;
  // alloc_info.pSetLayouts = &this->single_texture_descriptor_layout;
  //
  // VK_ASSERT(vkAllocateDescriptorSets(
  //     this->device,
  //     &alloc_info,
  //     &material->texture_descriptor));

  VkDescriptorImageInfo image_info = {};
  image_info.sampler = sampler;
  image_info.imageView = texture.image_view;
  image_info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

  for (auto &frame : this->frames) {
    VkWriteDescriptorSet texture_write = VkInit::write_descriptor_image(
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
        frame.texture_descriptor,
        &image_info,
        0,
        Material::get(material->material_name).value);

    vkUpdateDescriptorSets(this->device, 1, &texture_write, 0, nullptr);
  }
}

void Render::VulkanEngine::imgui_enqueue(std::function<void(void)> &&imgui_fn) {
  this->imgui_fns.push_back(imgui_fn);
}

void Render::VulkanEngine::submit_command(
    std::function<void(VkCommandBuffer)> &&function) {
  static bool init = false;
  if (!init) {
    VkFenceCreateInfo fence_info = VkInit::fence_create_info(false);
    VK_ASSERT(vkCreateFence(
        this->device,
        &fence_info,
        nullptr,
        &this->upload_context.upload_fence));

    VkCommandPoolCreateInfo pool_info = VkInit::command_pool_create_info(
        this->graphics_queue_family,
        VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);
    VK_ASSERT(vkCreateCommandPool(
        this->device,
        &pool_info,
        nullptr,
        &this->upload_context.command_pool));

    VkCommandBufferAllocateInfo alloc_info =
        VkInit::command_buffer_allocate_info(
            this->upload_context.command_pool,
            1,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    VK_ASSERT(vkAllocateCommandBuffers(
        this->device,
        &alloc_info,
        &this->upload_context.command_buffer));
    init = true;

    this->cleanup_fns.push([this]() {
      vkDestroyCommandPool(
          this->device,
          this->upload_context.command_pool,
          nullptr);
      vkDestroyFence(this->device, this->upload_context.upload_fence, nullptr);
    });
  }

  VkCommandBufferBeginInfo cmd_info = VkInit::command_buffer_begin_info(
      VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT);
  VK_ASSERT(
      vkBeginCommandBuffer(this->upload_context.command_buffer, &cmd_info));

  function(this->upload_context.command_buffer);

  VK_ASSERT(vkEndCommandBuffer(this->upload_context.command_buffer));

  VkSubmitInfo submit_info = VkInit::submit_info(
      nullptr,
      0,
      nullptr,
      0,
      &this->upload_context.command_buffer,
      nullptr);
  VK_ASSERT(vkQueueSubmit(
      this->graphics_queue,
      1,
      &submit_info,
      this->upload_context.upload_fence));

  vkWaitForFences(
      this->device,
      1,
      &this->upload_context.upload_fence,
      true,
      1000000000);
  vkResetFences(this->device, 1, &this->upload_context.upload_fence);
  vkResetCommandPool(this->device, this->upload_context.command_pool, 0);
}

Result<AllocatedImage>
Render::VulkanEngine::load_texture_asset(const std::string &path) {
  auto res_asset = files::load_file(path);

  if (res_asset.is_error) {
    return Result<AllocatedImage>::err(res_asset.msg);
  }

  MutBuf<uint8_t> mutbuf(res_asset.value);

  if ((AssetType)Serialize::deserialize_u32(mutbuf) != AssetType::Texture) {
    return Result<AllocatedImage>::err("Invalid texture type");
  }

  uint32_t width = Serialize::deserialize_u32(mutbuf);
  uint32_t height = Serialize::deserialize_u32(mutbuf);
  uint32_t size = Serialize::deserialize_u32(mutbuf);
  if (size != width * height * 4) {
    return Result<AllocatedImage>::err(
        "{} * {} (w * h) != {}",
        width,
        height,
        size);
  }

  uint32_t image_size = width * height * 4;
  VkFormat image_format = VK_FORMAT_R8G8B8A8_SRGB;

  AllocatedBuffer staging_buffer = VkInit::buffer(
      this->allocator,
      image_size,
      VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
      VMA_MEMORY_USAGE_CPU_ONLY);

  void *data;
  vmaMapMemory(this->allocator, staging_buffer.allocation, &data);

  std::memcpy(data, mutbuf.data(), static_cast<uint32_t>(image_size));

  vmaUnmapMemory(this->allocator, staging_buffer.allocation);

  VkExtent3D extent = {};
  extent.width = static_cast<uint32_t>(width);
  extent.height = static_cast<uint32_t>(height);
  extent.depth = 1;

  VkImageCreateInfo image_info = VkInit::image_create_info(
      image_format,
      VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
      extent);

  AllocatedImage image = {};

  VmaAllocationCreateInfo alloc_info = {};
  alloc_info.usage = VMA_MEMORY_USAGE_GPU_ONLY;

  vmaCreateImage(
      this->allocator,
      &image_info,
      &alloc_info,
      &image.image,
      &image.allocation,
      nullptr);

  this->submit_command([=](VkCommandBuffer cmd) {
    VkImageSubresourceRange range = {};
    range.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    range.baseMipLevel = 0;
    range.levelCount = 1;
    range.baseArrayLayer = 0;
    range.layerCount = 1;

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    barrier.image = image.image;
    barrier.subresourceRange = range;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &barrier);

    VkBufferImageCopy copy = {};
    copy.bufferOffset = 0;
    copy.bufferRowLength = 0;
    copy.bufferImageHeight = 0;
    copy.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    copy.imageSubresource.mipLevel = 0;
    copy.imageSubresource.baseArrayLayer = 0;
    copy.imageSubresource.layerCount = 1;
    copy.imageExtent = extent;

    vkCmdCopyBufferToImage(
        cmd,
        staging_buffer.buffer,
        image.image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &copy);

    VkImageMemoryBarrier readable = barrier;
    readable.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
    readable.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    readable.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
    readable.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

    vkCmdPipelineBarrier(
        cmd,
        VK_PIPELINE_STAGE_TRANSFER_BIT,
        VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
        0,
        0,
        nullptr,
        0,
        nullptr,
        1,
        &readable);
  });

  vmaDestroyBuffer(
      this->allocator,
      staging_buffer.buffer,
      staging_buffer.allocation);

  return Result<AllocatedImage>::ok(image);
}

Render::Frame &Render::VulkanEngine::next_frame() {
  static uint32_t frame_number = 0;

  Frame &next = this->frames[frame_number];

  frame_number = (frame_number + 1) % Render::VulkanEngine::FRAMES_IN_FLIGHT;
  return next;
}
