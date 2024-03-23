#pragma once

#include "callback_handler.h"
#include "io/input_map.h"
#include "util/err.h"
#include "vk_types.h"
#include "window.h"

#include <vector>
#include <vulkan/vulkan_core.h>

namespace Render {

class VulkanEngine {
public:
  ~VulkanEngine();

  Err init(Dimensions dimensions, CallbackHandler *handler);

  void render();

  void resize(Dimensions dimensions);

  void poll_events();
  InputMap get_inputs();

private:
  void init_vulkan();
  void init_swapchain();
  void init_commands();
  void init_default_renderpass();
  void init_framebuffers();
  void init_sync_structs();
  void init_pipelines();

private:
  Window window{};
  Dimensions dimensions;
  uint32_t frame_number;

  // Vulkan Structs
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice gpu;
  VkDevice device;
  VkSurfaceKHR surface;

  VkSwapchainKHR swapchain;
  VkFormat swapchain_format;
  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_image_views;

  VkQueue graphics_queue;
  uint32_t graphics_queue_family;
  VkCommandPool command_pool;
  VkCommandBuffer main_command_buffer;

  VkRenderPass render_pass;
  std::vector<VkFramebuffer> frame_buffers;

  VkSemaphore present_semaphore;
  VkSemaphore render_semaphore;
  VkFence render_fence;

  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;
};

} // namespace Render
