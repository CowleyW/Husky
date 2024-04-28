#version 460

struct InstanceData {
  mat4 model;
  int tex_index;
  int mesh_index;
  int padding[2];
};

struct AABB {
  vec4 min;
  vec4 max;
};

struct MeshData {
  AABB aabb;
};

layout (set = 0, binding = 0) uniform CameraBuffer {
  mat4 viewproj;
} camera_data;

layout (std430, binding = 1) readonly buffer MeshBuffer {
  MeshData meshes[];
};

layout(binding = 2) readonly buffer InstanceBuffer {
  InstanceData instances[];
} instance_data;

const int indices[36] = int[36](
  0, 1, 3, 3, 1, 2,
  1, 5, 2, 2, 5, 6,
  5, 4, 6, 6, 4, 7,
  4, 0, 7, 7, 0, 3,
  3, 2, 7, 7, 2, 6,
  4, 5, 0, 0, 5, 1
);

vec3 aabb_point(vec3 min, vec3 max) {
  switch (indices[gl_VertexIndex]) {
    case 0: return min;
    case 1: return vec3(max.x, min.yz);
    case 2: return vec3(max.xy, min.z);
    case 3: return vec3(min.x, max.y, min.z);
    case 4: return vec3(min.xy, max.z);
    case 5: return vec3(max.x, min.y, max.z);
    case 6: return max;
    case 7: return vec3(min.x, max.yz);
  }
}

void main() {
  InstanceData inst = instance_data.instances[gl_InstanceIndex];
  MeshData data = meshes[inst.mesh_index];
  mat4 model = inst.model;
  mat4 transform = camera_data.viewproj * model;
  gl_Position = transform * vec4(aabb_point(data.aabb.min.xyz, data.aabb.max.xyz), 1.0f);
}
