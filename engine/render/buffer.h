#pragma once

#include <vk_mem_alloc.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

struct AllocatedBuffer {
  VkBuffer buffer;
  VmaAllocation allocation;

  static uint32_t padding_size(
      uint32_t original_size,
      const VkPhysicalDeviceProperties &properties) {
    // Efficient function to calculate padding size from:
    // https://github.com/SaschaWillems/Vulkan/tree/master/examples/dynamicuniformbuffer
    uint32_t min_alignment = properties.limits.minUniformBufferOffsetAlignment;
    if (min_alignment > 0) {
      return (original_size + min_alignment - 1) & ~(min_alignment - 1);
    } else {
      return original_size;
    }
  }

  void destroy(VmaAllocator &allocator) {
    vmaDestroyBuffer(allocator, this->buffer, this->allocation);
  }
};
