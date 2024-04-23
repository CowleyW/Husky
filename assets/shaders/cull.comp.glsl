#version 450

struct InInstanceData {
  vec3 position;
  int tex_index;
  vec3 rotation;
  int mesh_index;
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

layout (std430, binding = 2) buffer IndirectDraws {
  IndexedIndirectCommand draws[];
};

layout (binding = 3) uniform CameraBuffer {
  mat4 viewproj;
  vec4 frustums[6];
} camera_data;

layout(binding = 4) buffer DrawStats {
  uint draw_count;
} draw_stats;


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

bool is_visible(vec4 pos, float radius) {
  for (int i = 0; i < 6; i += 1) {
    if (dot(pos, camera_data.frustums[i]) + radius < 0.0f) {
      return false;
    }
  }

  return true;
}

layout (local_size_x = 16) in;

void main() {
  uint idx = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

  InInstanceData inst = in_instances[idx];

  vec4 pos = vec4(inst.position, 1.0f);
  float radius = max(max(inst.scale.x, inst.scale.y), inst.scale.z);

  if (is_visible(pos, radius)) {
    uint count = atomicAdd(draws[inst.mesh_index].instance_count, 1);
    uint mesh_idx = draws[inst.mesh_index].first_instance + count;

    mat4 model = mat4(1.0f) * scale(inst.scale) * rotate(inst.rotation) * translate(inst.position);

    out_instances[mesh_idx].model = model;
    out_instances[mesh_idx].tex_index = inst.tex_index;

    atomicAdd(draw_stats.draw_count, 1);
  }
}
