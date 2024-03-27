#include "raw_inputs.h"

#include <GLFW/glfw3.h>

bool RawInputs::is_key_down(KeyCode key) {
  return this->keys.test(key);
}

bool RawInputs::is_mouse_button_down(MouseButton button) {
  return this->mouse_buttons.test(button);
}
