add_library(glad
  include/glad/gl.h src/gl.c include/KHR/khrplatform.h)

target_include_directories(glad PRIVATE include/)
