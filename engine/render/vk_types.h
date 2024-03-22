#pragma once

#include "io/logging.h"

#include <vk_mem_alloc.h>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan.h>

#include <cstdint>

#define VK_ASSERT(x)                                                           \
  do {                                                                         \
    VkResult err = x;                                                          \
    if (err) {                                                                 \
      io::error("{}", string_VkResult(err));                                   \
    }                                                                          \
  } while (false)

struct Dimensions {
  uint32_t width;
  uint32_t height;
};

enum class ShaderType { Vertex = 0, Fragment };
