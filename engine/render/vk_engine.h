#pragma once

#include "callback_handler.h"
#include "ecs/scene.h"
#include "io/input_map.h"
#include "tri_mesh.h"
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

  void render(Scene &scene);

  void resize(Dimensions dimensions);

  void poll_events();

  InputMap get_inputs();

private:
  void init_vulkan();
  void init_allocator();
  void init_swapchain();
  void init_default_renderpass();
  void init_descriptors();
  void init_frames();
  void init_framebuffers();
  void init_pipelines();
  void init_meshes();
  void init_imgui();

  void upload_mesh(TriMesh &mesh);

  FrameData &next_frame();

private:
  static constexpr uint32_t FRAMES_IN_FLIGHT = 2;

  Window window{};
  Dimensions dimensions;
  uint32_t frame_number;

  TriMesh obj_mesh;

  SceneData scene_data;
  AllocatedBuffer scene_data_buffer;

  // Vulkan Structs
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice gpu;
  VkDevice device;
  VkSurfaceKHR surface;

  VkPhysicalDeviceProperties gpu_properties;

  VkSwapchainKHR swapchain;
  VkFormat swapchain_format;
  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_image_views;

  VkQueue graphics_queue;
  uint32_t graphics_queue_family;

  VkRenderPass render_pass;
  std::vector<VkFramebuffer> frame_buffers;

  VkDescriptorSetLayout global_descriptor_layout;
  VkDescriptorSetLayout object_descriptor_layout;
  VkDescriptorPool descriptor_pool;

  FrameData frames[FRAMES_IN_FLIGHT];

  VkPipeline mesh_pipeline;
  VkPipelineLayout mesh_pipeline_layout;

  VkImageView depth_image_view;
  AllocatedImage depth_image;
  VkFormat depth_format;

  VmaAllocator allocator;
};

} // namespace Render
