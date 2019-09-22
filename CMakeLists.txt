cmake_minimum_required(VERSION 3.5)

project(h-lang LANGUAGES CXX VERSION 0.0.1)

add_subdirectory(lib/Catch2)

add_executable(h-lang_tests test/test.cpp)
target_link_libraries(h-lang_tests Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(h-lang_tests)

