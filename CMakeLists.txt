cmake_minimum_required(VERSION 3.21)

project(Husky)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "-std=c++17 ${CMAKE_CXX_FLAGS}")

# Engine Library
add_library(engine
  # Core
  engine/core/application.h engine/core/application.cpp
  engine/core/client_app.h engine/core/client_app.cpp
  engine/core/server_app.h engine/core/server_app.cpp
  engine/core/types.h engine/core/def.h
  
  # Crypto
  engine/crypto/checksum.h engine/crypto/checksum.cpp

  # I/O
  engine/io/files.h engine/io/files.cpp
  engine/io/input_map.h engine/io/input_map.cpp
  engine/io/logging.h
  
  #Net
  engine/net/client.h engine/net/client.cpp
  engine/net/client_slot.h engine/net/client_slot.cpp
  # engine/net/connection.h engine/net/connection.cpp
  engine/net/listener.h engine/net/listener.cpp
  engine/net/sender.h engine/net/sender.cpp
  # engine/net/connection.h engine/net/connection.cpp
  engine/net/message.h engine/net/message.cpp
  engine/net/message_builder.h engine/net/message_builder.cpp
  engine/net/message_handler.h
  # engine/net/message_queue.h engine/net/message_queue.cpp
  engine/net/server.h engine/net/server.cpp

  # Render
  engine/render/window.h engine/render/window.cpp
  engine/render/context.h engine/render/context.cpp
  engine/render/shader.h engine/render/shader.cpp
  engine/render/callback_handler.h
  engine/render/gl_types.h

  #Util
  engine/util/err.h engine/util/result.h
  engine/util/serialize.h engine/util/serialize.cpp
  engine/util/buf.h
)

target_compile_definitions(engine PRIVATE ASSETS_PATH="${PROJECT_SOURCE_DIR}/assets/")
target_compile_features(engine PRIVATE cxx_std_17)

find_package(OpenGL REQUIRED)

add_subdirectory(lib/glfw)
add_subdirectory(lib/glad)
add_subdirectory(lib/fmt)
target_link_libraries(engine PUBLIC 
  fmt 
  glfw 
  OpenGL::GL 
  glad
  ws2_32
  wsock32
)

target_include_directories(engine PUBLIC 
  engine/ 
  lib/asio/asio/include
  lib/fmt/include/
  lib/glfw/include/ 
  lib/glad/include/)

target_compile_definitions(engine PRIVATE _WIN32_WINNT=0x0A00)

# Main Configuration
add_executable(runtime 
  runtime/main.cpp
)

target_compile_definitions(runtime PRIVATE _WIN32_WINNT=x0A00)

target_link_libraries(runtime PRIVATE engine)
target_include_directories(runtime PRIVATE runtime/ .)

# Test Configuration
add_subdirectory(lib/Catch2)

add_executable(tests 
  test/io/files.cpp
  test/net/message.cpp
  test/util/serialize.cpp
)

target_compile_features(tests PRIVATE cxx_std_17)

target_link_libraries(tests PRIVATE engine Catch2::Catch2WithMain)
target_include_directories(tests PRIVATE . lib/Catch2/src)

include(CTest)
include(Catch)
catch_discover_tests(tests)
