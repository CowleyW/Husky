#include "buffer.h"

VkDescriptorBufferInfo AllocatedBuffer::descriptor_info() {
  VkDescriptorBufferInfo info = {};
  info.buffer = this->buffer;
  info.offset = 0;
  info.range = range;

  return info;
}

void AllocatedBuffer::destroy(VmaAllocator &allocator) {
  vmaDestroyBuffer(allocator, this->buffer, this->allocation);
}

uint32_t AllocatedBuffer::padding_size(
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
