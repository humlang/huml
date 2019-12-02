cmake_minimum_required(VERSION 3.5)

project(h-lang LANGUAGES CXX VERSION 0.0.1)

set(h-lang_files src/reader.cpp
  src/token.cpp
  src/ast.cpp
  src/symbol.cpp
  src/source_range.cpp
  src/diagnostic.cpp
  src/stream_lookup.cpp
  src/argument_parser.cpp
  )

find_package(fmt REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# tests
find_package(Catch2 REQUIRED)

add_executable(h-lang_parsing_tests test/parsing_and_tokenization.cpp
  "${h-lang_files}")
target_compile_definitions(h-lang_parsing_tests PUBLIC -DH_LANG_TESTING)
target_link_libraries(h-lang_parsing_tests Catch2::Catch2 fmt::fmt stdc++fs)
set_property(TARGET h-lang_parsing_tests PROPERTY CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/lib/Catch2/contrib/")

include(CTest)
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(h-lang_parsing_tests)

# main program
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_executable(h-lang src/main.cpp "${h-lang_files}")
target_link_libraries(h-lang PRIVATE fmt::fmt-header-only stdc++fs)
set_property(TARGET h-lang PROPERTY CXX_STANDARD 17)

