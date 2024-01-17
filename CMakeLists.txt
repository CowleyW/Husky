cmake_minimum_required(VERSION 3.21)

project(Triton)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

# Engine Library
add_library(engine
  engine/io/logging.h engine/io/logging.cpp
  engine/core/window.h engine/core/window.cpp
)

find_package(OpenGL REQUIRED)

add_subdirectory(lib/glfw)
target_link_libraries(engine PUBLIC glfw OpenGL::GL)
target_include_directories(engine PUBLIC engine/ lib/glfw/include)

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
