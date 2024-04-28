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
  int mesh_index;
  int _padding[2];
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

layout (std140, binding = 3) uniform CullData {
  vec4 frustums[6];
  uint instance_count;
} cull_data;

layout (binding = 4) uniform CameraBuffer {
  mat4 viewproj;
} camera_data;

layout(binding = 5) buffer DrawStats {
  uint draw_count;
  uint aabb_vertices;
  uint precull_indices;
  uint postcull_indices;
} draw_stats;

struct AABB {
  vec4 min;
  vec4 max;
};

struct MeshData {
  AABB aabb;

  // uint index_count;
  // uint first_index;
  // uint vertex_offset;
  // uint _padding;
};

layout (std430, binding = 6) readonly buffer MeshBuffer {
  MeshData meshes[];
};

struct IndirectCommand {
  uint vertex_count; // 36 vertices
  uint instance_count;
  uint first_vertex; // 0
  uint first_instance; // 0
};

layout (binding = 7) buffer AABBDraws {
  IndirectCommand aabb_draws[];
};

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

bool test_circle(vec4 pos, float radius) {
  for (int i = 0; i < 6; i += 1) {
    if (dot(pos, cull_data.frustums[i]) + radius < 0.0f) {
      return false;
    }
  }

  return true;
}

bool within(float min, float test, float max) {
  return min <= test && test <= max;
}

bool test_aabb(mat4 mvp, AABB aabb) {
  vec4 corners[8] = vec4[8](
    vec4(aabb.min.xyz, 1.0f),
    vec4(aabb.max.x, aabb.min.yz, 1.0f),
    vec4(aabb.max.xy, aabb.min.z, 1.0f),
    vec4(aabb.min.x, aabb.max.y, aabb.min.z, 1.0f),
    vec4(aabb.min.xy, aabb.max.z, 1.0f),
    vec4(aabb.max.x, aabb.min.y, aabb.max.z, 1.0f),
    vec4(aabb.max.xyz, 1.0f),
    vec4(aabb.min.x, aabb.max.yz, 1.0f));

  bool inside = false;

  for (int i = 0; i < 8; i += 1) {
    vec4 corner = mvp * corners[i];

    inside = inside ||
      (within(-corner.w, corner.x, corner.w) && 
       within(-corner.w, corner.y, corner.w) && 
       within(0.0f, corner.z, corner.w));
  }
  
  return inside;
}

layout (local_size_x = 16) in;

void main() {
  uint idx = gl_GlobalInvocationID.x + gl_GlobalInvocationID.y * gl_NumWorkGroups.x * gl_WorkGroupSize.x;

  if (idx >= cull_data.instance_count) {
    return;
  }

  InInstanceData inst = in_instances[idx];

  atomicAdd(draw_stats.precull_indices, draws[inst.mesh_index].index_count);

  mat4 model = mat4(1.0f) * scale(inst.scale) * rotate(inst.rotation) * translate(inst.position);
  mat4 mvp = camera_data.viewproj * model;
  AABB aabb = meshes[inst.mesh_index].aabb;

  if (test_aabb(mvp, aabb)) {
    uint count = atomicAdd(draws[inst.mesh_index].instance_count, 1);
    uint mesh_idx = draws[inst.mesh_index].first_instance + count;


    out_instances[mesh_idx].model = model;
    out_instances[mesh_idx].tex_index = inst.tex_index;
    out_instances[mesh_idx].mesh_index = inst.mesh_index;

    atomicAdd(aabb_draws[inst.mesh_index].instance_count, 1);

    atomicAdd(draw_stats.draw_count, 1);
    atomicAdd(draw_stats.postcull_indices, draws[inst.mesh_index].index_count);
    atomicAdd(draw_stats.aabb_vertices, aabb_draws[inst.mesh_index].vertex_count);

  }
}
