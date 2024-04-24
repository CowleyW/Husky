#pragma once

#include "callback_handler.h"
#include "frame.h"
#include "io/input_map.h"
#include "material.h"
#include "tri_mesh.h"
#include "vk_types.h"
#include "window.h"

#include "entt/entity/fwd.hpp"
#include <functional>
#include <stack>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Render {

class VulkanEngine {
public:
  VulkanEngine(
      Dimensions dimensions = {1920, 1080},
      CallbackHandler *handler = nullptr);
  ~VulkanEngine();

  void render(entt::registry &registry);

  void resize(Dimensions dimensions);

  void poll_events();

  InputMap get_inputs();

  void upload_mesh(TriMesh *mesh);
  void upload_material(Material *material);

  void imgui_enqueue(std::function<void(void)> &&imgui_fn);

private:
  void destroy_swapchain();

  void init_vulkan();
  void init_allocator();
  void init_buffers();
  void init_compute();
  void init_sampler();
  void init_swapchain();
  void init_default_renderpass();
  void init_descriptors();
  void init_frames();
  void init_framebuffers();
  void init_pipelines();
  void init_imgui();

  uint32_t prepare_frame(Frame &frame);
  CameraData get_camera_data(entt::registry &registry, uint32_t total_objects);
  void prepare_imgui_data();

  void submit_command(std::function<void(VkCommandBuffer)> &&function);

  Result<AllocatedImage> load_texture_asset(const std::string &path);

  Frame &next_frame();

private:
  static constexpr uint32_t FRAMES_IN_FLIGHT = 2;
  static constexpr uint32_t MAX_INSTANCES = 50000;
  // We allocate 50mb to the master buffer. This buffer will contain both vertex
  // buffers and index buffers
  static constexpr uint32_t MASTER_BUFFER_SIZE = 50 * 1024 * 1024;

  Window window{};
  Dimensions dimensions;
  uint32_t frame_number;

  std::vector<std::function<void(void)>> imgui_fns;
  std::stack<std::function<void(void)>> cleanup_fns;

  std::unordered_map<std::string, Texture> textures;
  Material material;

  AllocatedBuffer vertex_buffer;
  uint32_t vertex_buffer_offset = 0;

  AllocatedBuffer index_buffer;
  uint32_t index_buffer_offset = 0;

  AllocatedBuffer indirect_commands_buffer;
  AllocatedBuffer indirect_count_buffer;

  Compute compute;

  SceneData scene_data;
  AllocatedBuffer scene_data_buffer;

  UploadContext upload_context;

  // Vulkan Structs
  VkInstance instance;
  VkDebugUtilsMessengerEXT debug_messenger;
  VkPhysicalDevice gpu;
  VkDevice device;
  VkSurfaceKHR surface;

  VkPhysicalDeviceProperties gpu_properties;

  VkSwapchainKHR swapchain;
  VkFormat swapchain_format;
  VkExtent2D swapchain_extent;
  std::vector<VkImage> swapchain_images;
  std::vector<VkImageView> swapchain_image_views;

  VkQueue graphics_queue;
  uint32_t graphics_queue_family;

  VkRenderPass render_pass;
  std::vector<VkFramebuffer> frame_buffers;

  VkDescriptorSetLayout global_descriptor_layout;
  VkDescriptorSetLayout object_descriptor_layout;
  VkDescriptorSetLayout texture_descriptor_layout;
  VkDescriptorPool descriptor_pool;

  Frame frames[FRAMES_IN_FLIGHT];

  VkPipeline mesh_pipeline;
  VkPipelineLayout mesh_pipeline_layout;
  VkSampler sampler;

  VkImageView depth_image_view;
  AllocatedImage depth_image;
  VkFormat depth_format;

  VmaAllocator allocator;

  // ImGUI Resources
  VkDescriptorPool imgui_descriptor_pool;

  DrawStats draw_stats;
};

} // namespace Render
