#include "vertex.h"

bool Vertex::operator==(const Vertex &other) const {
  return this->position == other.position && this->color == other.color &&
         this->normal == other.normal && this->uvs == other.uvs;
}

VertexInputDescription Vertex::get_description() {
  VertexInputDescription description;

  VkVertexInputBindingDescription per_vertex_binding = {};
  per_vertex_binding.binding = 0;
  per_vertex_binding.stride = sizeof(Vertex);
  per_vertex_binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

  description.bindings.push_back(per_vertex_binding);

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

  VkVertexInputAttributeDescription texture_attrib = {};
  texture_attrib.binding = 0;
  texture_attrib.location = 3;
  texture_attrib.format = VK_FORMAT_R32G32_SFLOAT;
  texture_attrib.offset = offsetof(Vertex, uvs);

  description.attributes.push_back(position_attrib);
  description.attributes.push_back(normal_attrib);
  description.attributes.push_back(color_attrib);
  description.attributes.push_back(texture_attrib);

  return description;
}
