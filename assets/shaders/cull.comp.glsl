#version 450

struct InstanceData {
  mat4 model;
  int tex_index;
  int _padding[3];
};

struct IndexedIndirectCommand {
  uint index_count;
  uint instance_count;
  uint first_index;
  uint vertex_offset;
  uint first_instance;
};

layout (std430, binding = 0) buffer Instances {
  InstanceData instances[];
};

// layout (std430, binding = 1) writeonly buffer IndirectDraws {
//   IndexedIndirectCommand draws[];
// };

// layout(binding = 1) buffer DrawStats {
//   uint draw_count;
// } draw_stats;

// struct LOD {
//   uint first_index;
//   uint index_count;
//   float distance;
//   float _padding;
// };

// layout(binding = 3) readonly buffer LODs {
//   LOD lods[];
// };

layout (local_size_x = 16) in;

void main() {
  uint idx = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

  instances[idx].model[3].y = (idx % 16) / 16.0f;

  // atomicAdd(draw_stats.draw_count, 1);
}
