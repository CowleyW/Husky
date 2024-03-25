#pragma once

#include "ecs/components.h"
#include "ecs/ecs_types.h"
#include "entity.h"

#include <bitset>
#include <iterator>
#include <vector>

class Scene {
public:
  struct Iterator : std::iterator_traits<uint64_t> {
    Iterator(
        const std::vector<Entity> &entities,
        std::bitset<MAX_COMPONENTS> component_mask,
        uint32_t index)
        : entities(entities),
          component_mask(component_mask),
          index(index) {
    }

    uint64_t operator*() {
      return this->entities[this->index].id;
    }

    bool operator==(const Iterator &other) {
      return this->index == other.index;
    }

    bool operator!=(const Iterator &other) {
      return this->index != other.index;
    }

    Iterator &operator++() {
      do {
        this->index += 1;
      } while (this->index < this->entities.size() && !this->valid());

      return *this;
    }

    const Iterator begin() const {
      return Scene::Iterator(this->entities, this->component_mask, 0);
    }

    const Iterator end() const {
      return Iterator(
          this->entities,
          this->component_mask,
          this->entities.size());
    }

  private:
    bool valid() {
      return (this->component_mask & this->entities[index].mask) ==
             this->component_mask;
    }

  private:
    const std::vector<Entity> &entities;
    const std::bitset<MAX_COMPONENTS> component_mask;
    uint32_t index;
  };

public:
  Scene();

  uint64_t new_entity();

  template <typename T>
  T *get(uint64_t id) {
    uint32_t component_id = T::id();

    return &this->components[component_id].get<T>(id);
  }

  template <typename T>
  void assign(uint64_t id, T component) {
    uint32_t component_id = T::id();

    this->components[component_id].get<T>(id) = component;
    this->entities[id].mask.set(component_id);
  }

  // Likely the worst piece of code ever constructed by a human being.
  template <typename... Components>
  Iterator view() {
    std::bitset<MAX_COMPONENTS> component_mask = {};
    uint32_t ids[] = {Components::id()...};
    for (uint32_t i = 0; i < (sizeof...(Components)); i += 1) {
      component_mask.set(ids[i]);
    }

    return Scene::Iterator(this->entities, component_mask, 0);
  }

private:
  std::vector<Entity> entities;

  std::vector<ComponentArray> components;
};
