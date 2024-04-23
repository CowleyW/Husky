#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct AllocatedBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;
  VkDeviceSize range;

  VkDescriptorBufferInfo descriptor_info();

  void destroy(VmaAllocator &allocator);

  static uint32_t padding_size(
      uint32_t original_size,
      const VkPhysicalDeviceProperties &properties);
};
