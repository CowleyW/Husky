cmake_minimum_required(VERSION 3.21)

project(Husky)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if (MSVC)
	set(CMAKE_CXX_STANDARD 17)
else ()
	set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")
endif()

# Engine Library
add_library(engine
  # Core
  engine/core/application.h engine/core/application.cpp
  engine/core/client_app.h engine/core/client_app.cpp
  engine/core/server_app.h engine/core/server_app.cpp
  engine/core/def.h
  engine/core/position.h engine/core/position.cpp
  engine/core/random.h engine/core/random.cpp
  engine/core/world_state.h engine/core/world_state.cpp
  
  # Crypto
  engine/crypto/checksum.h engine/crypto/checksum.cpp

  # ECS
  engine/ecs/components.h engine/ecs/components.cpp
  engine/ecs/ecs_types.h
  engine/ecs/entity.h engine/ecs/entity.cpp
  engine/ecs/scene.h engine/ecs/scene.cpp

  # I/O
  engine/io/files.h engine/io/files.cpp
  engine/io/input_map.h engine/io/input_map.cpp
  engine/io/logging.h
  engine/io/raw_inputs.h engine/io/raw_inputs.cpp
  
  #Net
  engine/net/client.h engine/net/client.cpp
  engine/net/client_slot.h engine/net/client_slot.cpp
  engine/net/listener.h engine/net/listener.cpp
  engine/net/sender.h engine/net/sender.cpp
  engine/net/message.h engine/net/message.cpp
  engine/net/message_builder.h engine/net/message_builder.cpp
  engine/net/message_handler.h
  engine/net/server.h engine/net/server.cpp
  engine/net/types.h

  # Render
  engine/render/callback_handler.h
  engine/render/tri_mesh.h engine/render/tri_mesh.cpp
  engine/render/pipeline_builder.h engine/render/pipeline_builder.cpp
  engine/render/shader.h engine/render/shader.cpp
  engine/render/vk_engine.h engine/render/vk_engine.cpp
  engine/render/vk_init.h engine/render/vk_init.cpp
  engine/render/vk_types.h
  engine/render/window.h engine/render/window.cpp

  #Util
  engine/util/err.h engine/util/result.h
  engine/util/serialize.h engine/util/serialize.cpp
  engine/util/buf.h)

target_compile_definitions(engine PRIVATE ASSETS_PATH="${PROJECT_SOURCE_DIR}/assets/")
target_compile_features(engine PRIVATE cxx_std_17)

find_package(Vulkan REQUIRED)

add_subdirectory(lib/glfw)
add_subdirectory(lib/glm)
add_subdirectory(lib/fmt)
add_subdirectory(lib/VulkanMemoryAllocator)
add_subdirectory(lib/vk-bootstrap)
add_subdirectory(lib/tinyobjloader)

# imgui
add_library(imgui STATIC 
  lib/imgui/imconfig.h
  lib/imgui/imgui.h lib/imgui/imgui.cpp 
  lib/imgui/imgui_demo.cpp lib/imgui/imgui_draw.cpp 
  lib/imgui/imgui_internal.h lib/imgui/imgui_tables.cpp
  lib/imgui/imgui_widgets.cpp lib/imgui/imstb_rectpack.h 
  lib/imgui/imstb_textedit.h lib/imgui/imstb_truetype.h
  lib/imgui/backends/imgui_impl_glfw.h lib/imgui/backends/imgui_impl_glfw.cpp
  lib/imgui/backends/imgui_impl_vulkan.h lib/imgui/backends/imgui_impl_vulkan.cpp)

target_include_directories(imgui PUBLIC lib/imgui)

target_link_libraries(imgui PUBLIC glfw Vulkan::Vulkan)

target_link_libraries(engine PUBLIC 
  fmt 
  glfw
  glm::glm
  Vulkan::Vulkan
  GPUOpen::VulkanMemoryAllocator
  vk-bootstrap::vk-bootstrap
  ws2_32
  wsock32
  imgui
  tinyobjloader
)

target_include_directories(engine PUBLIC 
  engine/ 
  lib/asio/asio/include/
  lib/fmt/include/
  lib/glfw/include/
  lib/vk-bootstrap/src/
  lib/VulkanMemoryAllocator/include/
  lib/Vulkan-Utility-Libraries/include/vulkan/
  lib/imgui
  lib/imgui/backends
  lib/tinyobjloader
  lib/stb)

target_compile_definitions(engine PRIVATE _WIN32_WINNT=0x0A00)

# Main Configuration
add_executable(runtime 
  runtime/main.cpp)

target_compile_definitions(runtime PRIVATE _WIN32_WINNT=x0A00)

target_link_libraries(runtime PRIVATE engine)
target_include_directories(runtime PRIVATE runtime/ .)

# Test Configuration
add_subdirectory(lib/Catch2)

add_executable(tests 
  test/io/files.cpp
  test/net/message.cpp
  test/util/serialize.cpp
  test/ecs/scene.cpp)

target_compile_features(tests PRIVATE cxx_std_17)

target_link_libraries(tests PRIVATE engine Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE . lib/Catch2/src)

include(CTest)
include(Catch)
catch_discover_tests(tests)
