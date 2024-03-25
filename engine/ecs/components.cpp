#include "components.h"
#include "ecs/ecs_types.h"

ComponentArray::ComponentArray() {
  this->data = nullptr;
}

ComponentArray::~ComponentArray() {
  delete[] this->data;
}
