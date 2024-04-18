#version 460 core

#extension GL_EXT_nonuniform_qualifier : require

layout (location = 0) in vec3 v_color;
layout (location = 1) in vec2 v_tex_coords;
layout (location = 2) flat in int v_tex_index;

layout (location = 0) out vec4 FragColor;

layout(set = 0, binding = 1) uniform SceneData {
  vec4 fog_color;
  vec4 fog_distances;
  vec4 ambient_color;
  vec4 sunlight_direction;
  vec4 sunlight_color;
} scene_data;

layout (set = 0, binding = 2) uniform sampler2D textures[];

void main() {
  vec3 tex_color = texture(textures[nonuniformEXT(v_tex_index)], v_tex_coords).xyz;
  FragColor = vec4(tex_color, 1.0f);
}
