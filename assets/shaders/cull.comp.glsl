#version 450

struct InInstanceData {
  vec3 position;
  int tex_index;
  vec3 rotation;
  int _padding;
  vec3 scale;
  int _padding2;
};

struct OutInstanceData {
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

layout (std430, binding = 0) buffer InInstanceBuffer {
  InInstanceData in_instances[];
};

layout (binding = 1) writeonly buffer OutInstanceBuffer {
  OutInstanceData out_instances[];
};

// layout (std430, binding = 1) writeonly buffer IndirectDraws {
//   IndexedIndirectCommand draws[];
// };

layout(binding = 2) buffer DrawStats {
  uint draw_count;
} draw_stats;

// struct LOD {
//   uint first_index;
//   uint index_count;
//   float distance;
//   float _padding;
// };

// layout(binding = 3) readonly buffer LODs {
//   LOD lods[];
// };

mat4 scale(vec3 s) {
  mat3 m = mat3(
    s.x, 0.0f, 0.0f,
    0.0f, s.y, 0.0f,
    0.0f, 0.0f, s.z
  );
  return mat4(m);
}

mat4 rotate(vec3 r) {
  return mat4(1.0f);
}

mat4 translate(vec3 p) {
  mat4 m = mat4(1.0f);
  m[3].xyz = p;

  return m;
}

layout (local_size_x = 16) in;

void main() {
  uint idx = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

  InInstanceData inst = in_instances[idx];

  mat4 model = mat4(1.0f) * scale(inst.scale) * rotate(inst.rotation) * translate(inst.position);

  out_instances[idx].model = model;
  out_instances[idx].tex_index = inst.tex_index;

  atomicAdd(draw_stats.draw_count, 1);
}
