#include "mesh.h"
#include <cstddef>
#include <vulkan/vulkan_core.h>

VertexInputDescription Vertex::get_description() {
  VertexInputDescription description;

  VkVertexInputBindingDescription main_binding = {};
  main_binding.binding = 0;
  main_binding.stride = sizeof(Vertex);
  main_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  description.bindings.push_back(main_binding);

  VkVertexInputAttributeDescription position_attrib = {};
  position_attrib.binding = 0;
  position_attrib.location = 0;
  position_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
  position_attrib.offset = offsetof(Vertex, position);

  VkVertexInputAttributeDescription normal_attrib = {};
  normal_attrib.binding = 0;
  normal_attrib.location = 1;
  normal_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
  normal_attrib.offset = offsetof(Vertex, normal);

  VkVertexInputAttributeDescription color_attrib = {};
  color_attrib.binding = 0;
  color_attrib.location = 2;
  color_attrib.format = VK_FORMAT_R32G32B32_SFLOAT;
  color_attrib.offset = offsetof(Vertex, color);

  description.attributes.push_back(position_attrib);
  description.attributes.push_back(normal_attrib);
  description.attributes.push_back(color_attrib);

  return description;
}
