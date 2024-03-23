#include "shader.h"

#include "io/files.h"
#include "vk_types.h"
#include <vulkan/vulkan_core.h>

Err Shader::load_shader_module(
    const std::string &path,
    VkDevice device,
    VkShaderModule *out_shader) {
  auto res_source = files::load_spirv_file(path);

  if (res_source.is_error) {
    return Err::err(res_source.msg);
  }

  std::vector<uint32_t> source = res_source.value;

  VkShaderModuleCreateInfo create_info = {};
  create_info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  create_info.pNext = nullptr;
  create_info.codeSize = source.size() * sizeof(uint32_t);
  create_info.pCode = source.data();

  if (vkCreateShaderModule(device, &create_info, nullptr, out_shader) !=
      VK_SUCCESS) {
    return Err::err("Failed to create shader module");
  }

  return Err::ok();
}
