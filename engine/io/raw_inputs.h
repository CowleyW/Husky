#pragma once

#include <bitset>

typedef int32_t KeyCode;
typedef int32_t MouseButton;

struct RawInputs {
  // GLFW keycodes range from -1 (UNKNOWN) to 348 (MENU). While a majority of
  // these bits will be unused, it is more convenient to simply store the keys
  // in a sparse array
  std::bitset<349> keys;

  // Likewise, mouse buttons are in the range 0 to 7.
  std::bitset<8> mouse_buttons;

  double mouse_x;
  double mouse_y;

  double mouse_dx;
  double mouse_dy;

  bool is_key_down(KeyCode key);
  bool is_mouse_button_down(MouseButton button);
};
