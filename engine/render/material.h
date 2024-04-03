#pragma once

#include "util/result.h"

#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

typedef uint32_t MaterialHandle;

struct Material {
  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;
  VkDescriptorSet texture_descriptor;

  static Result<Material *> get(MaterialHandle handle);

private:
  static MaterialHandle fresh_handle();

private:
  static std::vector<std::pair<MaterialHandle, Material>> materials;
};
