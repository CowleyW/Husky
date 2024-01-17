cmake_minimum_required(VERSION 3.22)

project(Triton)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)

# Main Configuration
add_executable(triton 
  src/main.cpp 
  src/io/logging.h src/io/logging.cpp
  src/core/window.h src/core/window.cpp
)

find_package(OpenGL REQUIRED)

add_subdirectory(lib/glfw)
target_link_libraries(triton PRIVATE glfw OpenGL::GL)
target_include_directories(triton PRIVATE src/ lib/glfw/include)

# Test Configuration
add_subdirectory(lib/Catch2)

add_executable(triton_tests src/io/logging.h src/io/logging.cpp test/io/logging.cpp)
target_link_libraries(triton_tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(triton_tests PRIVATE . lib/Catch2/src)

include(CTest)
include(Catch)
catch_discover_tests(triton_tests)
