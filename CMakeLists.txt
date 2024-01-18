cmake_minimum_required(VERSION 3.21)

project(Triton)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

# Engine Library
add_library(engine
  engine/io/logging.h engine/io/logging.cpp
  engine/core/application.h engine/core/application.cpp
  engine/render/window.h engine/render/window.cpp
  engine/render/context.h engine/render/context.cpp
  engine/render/callback_handler.h
  engine/render/gl_types.h
  engine/util/err.h
)

find_package(OpenGL REQUIRED)

add_subdirectory(lib/glfw)
add_subdirectory(lib/glad)
target_link_libraries(engine PUBLIC glfw OpenGL::GL glad)
target_include_directories(engine PUBLIC engine/ lib/glfw/include/ lib/glad/include/)

# Main Configuration
add_executable(runtime 
  runtime/main.cpp
)

target_link_libraries(runtime PRIVATE engine)
target_include_directories(runtime PRIVATE runtime/ .)

# Test Configuration
add_subdirectory(lib/Catch2)

add_executable(triton_tests test/io/logging.cpp)
target_link_libraries(triton_tests PRIVATE engine Catch2::Catch2WithMain)
target_include_directories(triton_tests PRIVATE . lib/Catch2/src)

include(CTest)
include(Catch)
catch_discover_tests(triton_tests)
