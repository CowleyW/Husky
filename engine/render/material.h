#pragma once

#include "util/result.h"

#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

typedef uint32_t MaterialHandle;

struct Material {
  static constexpr MaterialHandle NULL_HANDLE = 0;

public:
  VkPipeline pipeline;
  VkPipelineLayout pipeline_layout;
  VkDescriptorSet texture_descriptor;

  std::string material_name;

  static Result<Material *> get(MaterialHandle handle);
  static Result<MaterialHandle> get(const std::string &path);

private:
  static MaterialHandle fresh_handle();

private:
  static std::vector<std::pair<MaterialHandle, Material>> materials;
};
