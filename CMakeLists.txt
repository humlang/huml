cmake_minimum_required(VERSION 3.5)

project(ppparse LANGUAGES CXX VERSION 0.0.1)

add_subdirectory(lib/Catch2)

add_executable(ppparse_tests test/test.cpp)
target_link_libraries(ppparse_tests Catch2::Catch2)

include(CTest)
include(Catch)
catch_discover_tests(ppparse_tests)

