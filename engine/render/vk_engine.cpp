#include "vk_engine.h"

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/fwd.hpp"
#include "io/logging.h"
#include "render/callback_handler.h"
#include "render/pipeline_builder.h"
#include "render/shader.h"
#include "render/tri_mesh.h"
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
      this->obj_mesh.vertex_buffer.buffer,
      this->obj_mesh.vertex_buffer.allocation);
  vmaDestroyBuffer(
      this->allocator,
      this->scene_data_buffer.buffer,
      this->scene_data_buffer.allocation);
  vmaDestroyImage(
      this->allocator,
      this->depth_image.image,
      this->depth_image.allocation);

  vkDestroyDescriptorSetLayout(
      this->device,
      this->global_descriptor_layout,
      nullptr);
  vkDestroyDescriptorSetLayout(
      this->device,
      this->object_descriptor_layout,
      nullptr);
  vkDestroyDescriptorPool(this->device, this->descriptor_pool, nullptr);

  for (auto &frame : this->frames) {
    vkDestroyCommandPool(this->device, frame.command_pool, nullptr);
    vkDestroySemaphore(this->device, frame.present_semaphore, nullptr);
    vkDestroySemaphore(this->device, frame.render_semaphore, nullptr);
    vkDestroyFence(this->device, frame.render_fence, nullptr);
    vmaDestroyBuffer(
        this->allocator,
        frame.camera_buffer.buffer,
        frame.camera_buffer.allocation);
    vmaDestroyBuffer(
        this->allocator,
        frame.object_buffer.buffer,
        frame.object_buffer.allocation);
  }

  vmaDestroyAllocator(this->allocator);

  vkDestroyPipeline(this->device, this->mesh_pipeline, nullptr);
  vkDestroyPipelineLayout(this->device, this->mesh_pipeline_layout, nullptr);

  vkDestroySwapchainKHR(this->device, this->swapchain, nullptr);

  vkDestroyRenderPass(this->device, this->render_pass, nullptr);

  for (uint32_t i = 0; i < this->swapchain_image_views.size(); i += 1) {
    vkDestroyFramebuffer(this->device, this->frame_buffers[i], nullptr);
    vkDestroyImageView(this->device, this->swapchain_image_views[i], nullptr);
  }
  vkDestroyImageView(this->device, this->depth_image_view, nullptr);

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
  this->init_allocator();
  this->init_descriptors();
  this->init_frames();
  this->init_swapchain();
  this->init_default_renderpass();
  this->init_framebuffers();
  this->init_pipelines();
  this->init_meshes();

  return Err::ok();
}

void Render::VulkanEngine::render(Scene &scene) {
  FrameData &frame = this->next_frame();
  VK_ASSERT(
      vkWaitForFences(this->device, 1, &frame.render_fence, true, 1000000000));
  VK_ASSERT(vkResetFences(this->device, 1, &frame.render_fence));

  uint32_t next_image_index;
  VK_ASSERT(vkAcquireNextImageKHR(
      this->device,
      this->swapchain,
      1000000000,
      frame.present_semaphore,
      nullptr,
      &next_image_index));

  VK_ASSERT(vkResetCommandBuffer(frame.main_command_buffer, 0));

  VkCommandBufferBeginInfo cmd_info = VkInit::command_buffer_begin_info();
  VK_ASSERT(vkBeginCommandBuffer(frame.main_command_buffer, &cmd_info));

  std::vector<VkClearValue> clear_values;
  float b = std::abs(std::sin(this->frame_number / 120.0f));
  VkClearValue clear_value = {};
  clear_value.color = {{b, b, b, 1.0f}};
  VkClearValue depth_value = {};
  depth_value.depthStencil = {1.0f, 0};

  clear_values.push_back(clear_value);
  clear_values.push_back(depth_value);

  VkRenderPassBeginInfo render_pass_info = VkInit::render_pass_begin_info(
      this->render_pass,
      this->frame_buffers[next_image_index],
      &clear_values,
      this->dimensions);

  vkCmdBeginRenderPass(
      frame.main_command_buffer,
      &render_pass_info,
      VK_SUBPASS_CONTENTS_INLINE);

  vkCmdBindPipeline(
      frame.main_command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->mesh_pipeline);

  this->scene_data.ambient_color = {b, b, b, 1};

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

  vkCmdBindDescriptorSets(
      frame.main_command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->mesh_pipeline_layout,
      0,
      1,
      &frame.global_descriptor,
      1,
      &buffer_offset);

  vkCmdBindDescriptorSets(
      frame.main_command_buffer,
      VK_PIPELINE_BIND_POINT_GRAPHICS,
      this->mesh_pipeline_layout,
      1,
      1,
      &frame.object_descriptor,
      0,
      nullptr);

  VkDeviceSize offset = 0;
  vkCmdBindVertexBuffers(
      frame.main_command_buffer,
      0,
      1,
      &this->obj_mesh.vertex_buffer.buffer,
      &offset);

  uint32_t cam_id;
  for (uint32_t id : scene.view<Camera, Transform>()) {
    cam_id = id;
  }
  Camera *camera = scene.get<Camera>(cam_id);
  Transform *t = scene.get<Transform>(cam_id);
  glm::mat4 viewproj = camera->calc_viewproj(t->position);

  CameraData camera_data = {viewproj};

  void *data;
  vmaMapMemory(this->allocator, frame.camera_buffer.allocation, &data);

  std::memcpy(data, &camera_data, sizeof(CameraData));

  vmaUnmapMemory(this->allocator, frame.camera_buffer.allocation);

  void *object_data;
  vmaMapMemory(this->allocator, frame.object_buffer.allocation, &object_data);

  uint32_t current_index = 0;
  ObjectData *object_ssbo = (ObjectData *)object_data;
  TriMesh *current_mesh = nullptr;
  for (uint32_t id : scene.view<Mesh, Transform>()) {
    Transform *transform = scene.get<Transform>(id);
    Mesh *mesh = scene.get<Mesh>(id);

    if (current_mesh == nullptr) {
      current_mesh = mesh->mesh;
    }

    if (mesh->mesh == current_mesh && mesh->visible) {
      glm::mat4 model = transform->get_matrix();
      object_ssbo[current_index].model = model;
      current_index += 1;
    }
  }

  vmaUnmapMemory(this->allocator, frame.object_buffer.allocation);

  glm::mat4 model = glm::rotate(
      glm::mat4(1.0f),
      glm::radians(this->frame_number * 0.4f),
      glm::vec3(0, 1, 0));
  MeshPushConstant push_constant = {};
  push_constant.matrix = model;
  vkCmdPushConstants(
      frame.main_command_buffer,
      this->mesh_pipeline_layout,
      VK_SHADER_STAGE_VERTEX_BIT,
      0,
      sizeof(MeshPushConstant),
      &push_constant);

  // Change last parameter when abstracting to rendering more objects
  vkCmdDraw(
      frame.main_command_buffer,
      this->obj_mesh.vertices.size(),
      current_index,
      0,
      0);

  vkCmdEndRenderPass(frame.main_command_buffer);
  VK_ASSERT(vkEndCommandBuffer(frame.main_command_buffer));

  VkPipelineStageFlags wait_stage =
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

  VkSubmitInfo submit_info = VkInit::submit_info(
      &frame.present_semaphore,
      &frame.render_semaphore,
      &frame.main_command_buffer,
      &wait_stage);
  VK_ASSERT(
      vkQueueSubmit(this->graphics_queue, 1, &submit_info, frame.render_fence));

  VkPresentInfoKHR present_info = VkInit::present_info(
      &this->swapchain,
      &frame.render_semaphore,
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
  VkPhysicalDeviceShaderDrawParameterFeatures features = {};
  features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_DRAW_PARAMETER_FEATURES;
  features.pNext = nullptr;
  features.shaderDrawParameters = VK_TRUE;
  vkb::Device device = device_builder.add_pNext(&features).build().value();

  this->gpu = gpu.physical_device;
  this->device = device.device;
  this->gpu_properties = device.physical_device.properties;

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
}

void Render::VulkanEngine::init_descriptors() {
  std::vector<VkDescriptorPoolSize> sizes = {
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 10},
      {VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 10},
      {VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 10}};

  VkDescriptorPoolCreateInfo pool_info = {};
  pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
  pool_info.flags = 0;
  pool_info.maxSets = 10;
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

  VkDescriptorSetLayoutBinding bindings[] = {
      camera_buffer_binding,
      scene_buffer_binding};

  VkDescriptorSetLayoutCreateInfo set0_info = {};
  set0_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set0_info.pNext = nullptr;
  set0_info.flags = 0;
  set0_info.bindingCount = 2;
  set0_info.pBindings = bindings;

  vkCreateDescriptorSetLayout(
      this->device,
      &set0_info,
      nullptr,
      &this->global_descriptor_layout);

  VkDescriptorSetLayoutBinding object_buffer_binding =
      VkInit::descriptor_set_layout_binding(
          VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
          VK_SHADER_STAGE_VERTEX_BIT,
          0);

  VkDescriptorSetLayoutCreateInfo set1_info = {};
  set1_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
  set1_info.pNext = nullptr;
  set1_info.flags = 0;
  set1_info.bindingCount = 1;
  set1_info.pBindings = &object_buffer_binding;

  vkCreateDescriptorSetLayout(
      this->device,
      &set1_info,
      nullptr,
      &this->object_descriptor_layout);

  const uint32_t scene_data_size =
      Render::VulkanEngine::FRAMES_IN_FLIGHT *
      AllocatedBuffer::padding_size(sizeof(SceneData), this->gpu_properties);
  this->scene_data_buffer = VkInit::buffer(
      this->allocator,
      scene_data_size,
      VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);
}

void Render::VulkanEngine::init_frames() {
  VkCommandPoolCreateInfo command_pool_info = VkInit::command_pool_create_info(
      this->graphics_queue_family,
      VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT);

  VkFenceCreateInfo fence_create_info = VkInit::fence_create_info();
  VkSemaphoreCreateInfo semaphore_create_info = VkInit::semaphore_create_info();

  for (auto &frame : this->frames) {
    VK_ASSERT(vkCreateFence(
        this->device,
        &fence_create_info,
        nullptr,
        &frame.render_fence));

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

    VK_ASSERT(vkCreateCommandPool(
        this->device,
        &command_pool_info,
        nullptr,
        &frame.command_pool));

    VkCommandBufferAllocateInfo alloc_info =
        VkInit::command_buffer_allocate_info(
            frame.command_pool,
            1,
            VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VK_ASSERT(vkAllocateCommandBuffers(
        this->device,
        &alloc_info,
        &frame.main_command_buffer));

    frame.camera_buffer = VkInit::buffer(
        this->allocator,
        sizeof(CameraData),
        VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);
    frame.object_buffer = VkInit::buffer(
        this->allocator,
        // SSBO SIZE
        3 * sizeof(ObjectData),
        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
        VMA_MEMORY_USAGE_CPU_TO_GPU);

    VkDescriptorSetAllocateInfo global_set_alloc = {};
    global_set_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    global_set_alloc.pNext = nullptr;
    global_set_alloc.descriptorPool = this->descriptor_pool;
    global_set_alloc.descriptorSetCount = 1;
    global_set_alloc.pSetLayouts = &this->global_descriptor_layout;

    vkAllocateDescriptorSets(
        this->device,
        &global_set_alloc,
        &frame.global_descriptor);

    VkDescriptorSetAllocateInfo object_set_alloc = {};
    object_set_alloc.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    object_set_alloc.pNext = nullptr;
    object_set_alloc.descriptorPool = this->descriptor_pool;
    object_set_alloc.descriptorSetCount = 1;
    object_set_alloc.pSetLayouts = &this->object_descriptor_layout;

    vkAllocateDescriptorSets(
        this->device,
        &object_set_alloc,
        &frame.object_descriptor);

    VkDescriptorBufferInfo camera_info = {};
    camera_info.buffer = frame.camera_buffer.buffer;
    camera_info.offset = 0;
    camera_info.range = sizeof(CameraData);

    VkDescriptorBufferInfo scene_info = {};
    scene_info.buffer = this->scene_data_buffer.buffer;
    scene_info.offset = 0;
    scene_info.range = sizeof(SceneData);

    VkDescriptorBufferInfo object_info = {};
    object_info.buffer = frame.object_buffer.buffer;
    object_info.offset = 0;
    // SSBO SIZE
    object_info.range = 3 * sizeof(ObjectData);

    VkWriteDescriptorSet camera_set = VkInit::write_descriptor_set(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
        frame.global_descriptor,
        &camera_info,
        0);

    VkWriteDescriptorSet scene_set = VkInit::write_descriptor_set(
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC,
        frame.global_descriptor,
        &scene_info,
        1);

    VkWriteDescriptorSet object_set = VkInit::write_descriptor_set(
        VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
        frame.object_descriptor,
        &object_info,
        0);

    VkWriteDescriptorSet write_sets[] = {camera_set, scene_set, object_set};

    vkUpdateDescriptorSets(this->device, 3, write_sets, 0, nullptr);
  }
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
  VkShaderModule rainbow_frag;
  VkShaderModule mesh_vert;

  Err err = Shader::load_shader_module(
      "shaders/lighting_frag.spv",
      this->device,
      &rainbow_frag);
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

  PipelineBuilder builder;

  std::vector<VkPushConstantRange> push_constant(1);
  push_constant[0].offset = 0;
  push_constant[0].size = sizeof(MeshPushConstant);
  push_constant[0].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

  std::vector<VkDescriptorSetLayout> descriptor_layouts;
  descriptor_layouts.push_back(this->global_descriptor_layout);
  descriptor_layouts.push_back(this->object_descriptor_layout);

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
              mesh_vert))
          .add_shader_stage(VkInit::pipeline_shader_stage_create_info(
              VK_SHADER_STAGE_FRAGMENT_BIT,
              rainbow_frag))
          .with_vertex_input(
              VkInit::vertex_input_state_create_info(&vertex_input))
          .with_pipeline_layout(this->mesh_pipeline_layout)
          .with_input_assembly(VkInit::input_assembly_state_create_info(
              VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST))
          .with_viewport(viewport)
          .with_scissor(scissor)
          .with_rasterizer(
              VkInit::rasterization_state_create_info(VK_POLYGON_MODE_FILL))
          .with_color_blend_attachment(VkInit::color_blend_attachment_state())
          .with_multisample(VkInit::multisample_state_create_info())
          .with_depth_stencil(VkInit::depth_stencil_create_info(
              true,
              true,
              VK_COMPARE_OP_LESS_OR_EQUAL))
          .build(this->device, this->render_pass);

  vkDestroyShaderModule(device, rainbow_frag, nullptr);
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
  auto res_mesh = TriMesh::load_from_obj("objs/mech_golem.obj");
  if (res_mesh.is_error) {
    io::error(res_mesh.msg);
  }

  this->obj_mesh = res_mesh.value;

  this->upload_mesh(this->obj_mesh);
  io::info("num vertices: {}", this->obj_mesh.vertices.size());
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

void Render::VulkanEngine::upload_mesh(TriMesh &mesh) {
  mesh.vertex_buffer = VkInit::buffer(
      this->allocator,
      mesh.vertices.size() * sizeof(Vertex),
      VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
      VMA_MEMORY_USAGE_CPU_TO_GPU);

  void *data;
  vmaMapMemory(this->allocator, mesh.vertex_buffer.allocation, &data);

  memcpy(data, mesh.vertices.data(), mesh.size());

  vmaUnmapMemory(allocator, mesh.vertex_buffer.allocation);
}

FrameData &Render::VulkanEngine::next_frame() {
  static uint32_t frame_number = 0;

  FrameData &next = this->frames[frame_number];

  frame_number = (frame_number + 1) % Render::VulkanEngine::FRAMES_IN_FLIGHT;
  return next;
}
