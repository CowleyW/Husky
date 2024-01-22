cmake_minimum_required(VERSION 3.21)

project(Triton)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")

# Engine Library
add_library(engine
  engine/io/logging.h
  engine/io/files.h engine/io/files.cpp
  engine/core/application.h engine/core/application.cpp
  engine/core/types.h
  engine/render/window.h engine/render/window.cpp
  engine/render/context.h engine/render/context.cpp
  engine/render/shader.h engine/render/shader.cpp
  engine/render/callback_handler.h
  engine/render/gl_types.h
  engine/util/err.h engine/util/result.h
)

target_compile_definitions(engine PRIVATE ASSETS_PATH="${PROJECT_SOURCE_DIR}/assets/")
target_compile_features(engine PRIVATE cxx_std_17)

find_package(OpenGL REQUIRED)

add_subdirectory(lib/glfw)
add_subdirectory(lib/glad)
add_subdirectory(lib/fmt)
target_link_libraries(engine PUBLIC fmt glfw OpenGL::GL glad)
target_include_directories(engine PUBLIC 
  engine/ 
  lib/fmt/include/
  lib/glfw/include/ 
  lib/glad/include/)

# Main Configuration
add_executable(runtime 
  runtime/main.cpp
)

target_link_libraries(runtime PRIVATE engine)
target_include_directories(runtime PRIVATE runtime/ .)

# Test Configuration
add_subdirectory(lib/Catch2)

add_executable(tests 
  test/io/logging.cpp test/io/files.cpp
)

target_compile_features(tests PRIVATE cxx_std_17)

target_link_libraries(tests PRIVATE engine Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE . lib/Catch2/src)

include(CTest)
include(Catch)
catch_discover_tests(tests)
