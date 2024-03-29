#pragma once

#include "ecs/ecs_types.h"
#include "glm/ext/vector_float3.hpp"
#include "render/tri_mesh.h"

#include "glm/gtc/matrix_transform.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/vec3.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include "glm/gtx/quaternion.hpp"

class ComponentArray {
public:
  ComponentArray();
  ~ComponentArray();

  template <typename T>
  T &get(uint32_t index) {
    if (this->data == nullptr) {
      this->data = new uint8_t[MAX_ENTITIES * sizeof(T)];
    }
    return ((T *)this->data)[index];
  }

private:
  uint8_t *data;
  uint32_t size;
  uint32_t capacity;
};

struct Transform {
  COMPONENT_DECLARATION(0);

  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 scale;

  glm::mat4 get_matrix() const {
    return glm::translate(glm::mat4(1.0f), this->position) *
           glm::toMat4(glm::quat(this->rotation)) *
           glm::scale(glm::mat4(1.0f), this->scale);
  }
};

struct Mesh {
  COMPONENT_DECLARATION(1);

  TriMesh *mesh;
  // Material *material;
  bool visible;
};

struct Camera {
  COMPONENT_DECLARATION(2);

  glm::vec3 forward;
  float fov;
  float z_near;
  float z_far;
  float yaw, pitch;

  glm::mat4 calc_viewproj(const glm::vec3 &world_pos) {
    glm::mat4 view =
        glm::lookAt(world_pos, world_pos + this->forward, {0.0f, 1.0f, 0.0f});
    glm::mat4 proj = glm::perspective(
        glm::radians(this->fov),
        1280 / 720.0f,
        this->z_near,
        this->z_far);
    proj[1][1] *= -1;

    return proj * view;
  }

  glm::vec3 calc_right() {
    return glm::normalize(glm::cross(this->forward, {0.0f, 1.0f, 0.0f}));
  }
};
