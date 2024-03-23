#pragma once

#include "util/err.h"

#include "vk_types.h"
#include <vulkan/vulkan_core.h>

namespace Shader {

Err load_shader_module(
    const std::string &path,
    VkDevice device,
    VkShaderModule *out_shader);

}
