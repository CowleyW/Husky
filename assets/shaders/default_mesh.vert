#version 450

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 v_color;

layout (location = 0) out vec3 a_color;

layout (set = 0, binding = 0) uniform CameraBuffer {
  mat4 view;
  mat4 proj;
  mat4 viewproj;
} camera_data;

layout (push_constant) uniform PushConstant {
  vec4 data;
  mat4 matrix;
} constants;

void main() {
  mat4 transform = camera_data.viewproj * constants.matrix;
  gl_Position = transform * vec4(v_position, 1.0f);
  a_color = v_color;
}
