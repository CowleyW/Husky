#version 450

layout (location = 0) in vec3 v_position;
layout (location = 1) in vec3 v_normal;
layout (location = 2) in vec3 v_color;

layout (location = 0) out vec3 a_color;

layout (push_constant) uniform constants {
  vec4 data;
  mat4 matrix;
} PushConstant;

void main() {
  gl_Position = PushConstant.matrix * vec4(v_position, 1.0f);
  a_color = v_color;
}
