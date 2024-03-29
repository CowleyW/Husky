#version 460 core

layout (location = 0) in vec3 v_color;

layout (location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform SceneData {
  vec4 fog_color;
  vec4 fog_distances;
  vec4 ambient_color;
  vec4 sunlight_direction;
  vec4 sunlight_color;
} scene_data;

void main() {
  FragColor = vec4(v_color, 1.0f);
}
