#include "vk_engine.h"

#include "render/callback_handler.h"
#include "render/vk_init.h"
#include "render/vk_types.h"

#include "VkBootstrap.h"
#include <cmath>
#include <cstdlib>
#include <vulkan/vulkan_core.h>

Render::VulkanEngine::~VulkanEngine() {
  // Make sure the GPU is not doing anything before we do any cleanup
  vkDeviceWaitIdle(this->device);

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
  clear_value.color = {{0.0f, 0.0f, b, 1.0f}};

  VkRenderPassBeginInfo render_pass_info = VkInit::render_pass_begin_info(
      this->render_pass,
      this->frame_buffers[next_image_index],
      &clear_value,
      this->dimensions);

  vkCmdBeginRenderPass(
      this->main_command_buffer,
      &render_pass_info,
      VK_SUBPASS_CONTENTS_INLINE);

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
