#version 450

struct InstanceData {
  mat4 model;
};

struct IndexedIndirectCommand {
  uint index_count;
  uint instance_count;
  uint first_index;
  uint vertex_offset;
  uint first_instance;
};

layout (std140, binding = 0) buffer Instances {
  InstanceData instances[];
};

layout (std430, binding = 1) writeonly buffer IndirectDraws {
  IndexedIndirectCommand draws[];
};

layout(binding = 2) buffer DrawStats {
  uint draw_count;
} draw_stats;

struct LOD {
  uint first_index;
  uint index_count;
  float distance;
  float _padding;
};

layout(binding = 3) readonly buffer LODs {
  LOD lods[];
};

layout (local_size_x = 16) in;

void main() {
  uint idx = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

  draws[idx].instance_count = 1;

  atomicAdd(draw_stats.draw_count, 1);

  draws[idx].first_index = lods[0].first_index;
  draws[idx].index_count = lods[0].index_count;
}
