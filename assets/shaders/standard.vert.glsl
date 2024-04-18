#version 460

layout (location = 0) in vec3 a_position;
layout (location = 1) in vec3 a_normal;
layout (location = 2) in vec3 a_color;
layout (location = 3) in vec2 a_tex_coords;

layout (location = 0) out vec3 v_color;
layout (location = 1) out vec2 v_tex_coords;
layout (location = 2) flat out int v_tex_index;

struct InstanceData {
  mat4 model;
  int tex_index;
  int padding[3];
};

layout (set = 0, binding = 0) uniform CameraBuffer {
  mat4 viewproj;
} camera_data;

layout(std430, set = 1, binding = 0) readonly buffer InstanceBuffer {
  InstanceData instances[];
} instance_data;

void main() {
  mat4 model = instance_data.instances[gl_InstanceIndex].model;
  mat4 transform = camera_data.viewproj * model;
  gl_Position = transform * vec4(a_position, 1.0f);
  v_color = a_color;
  v_tex_coords = a_tex_coords;
  v_tex_index = instance_data.instances[gl_InstanceIndex].tex_index;
}
