cmake_minimum_required(VERSION 3.22)

project(Draco)

add_executable(Draco main.cpp)

add_subdirectory(lib/Catch2)

add_executable(Tests test/test.cpp)
target_link_libraries(Tests PRIVATE Catch2::Catch2WithMain)
target_include_directories(Tests PRIVATE lib/Catch2/src)

include(CTest)
include(Catch)
catch_discover_tests(Tests)
