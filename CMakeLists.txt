cmake_minimum_required(VERSION 3.5)

project(hx-lang LANGUAGES CXX VERSION 0.0.1)

set(hx-lang_files src/reader.cpp
  src/token.cpp
  src/ast.cpp
  src/symbol.cpp
  src/source_range.cpp
  src/diagnostic.cpp
  src/stream_lookup.cpp
  )

set(hx-lang_vm_files src/vm.cpp
  src/repl.cpp
  )

find_package(fmt REQUIRED)
find_package(Threads REQUIRED)

set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

# tests
find_package(Catch2 REQUIRED)

## unit tests
add_executable(hx-lang_parsing_tests test/parsing_and_tokenization.cpp
  "${hx-lang_files}")
target_compile_definitions(hx-lang_parsing_tests PUBLIC -DH_LANG_TESTING)
target_link_libraries(hx-lang_parsing_tests Catch2::Catch2 fmt::fmt-header-only stdc++fs ${CMAKE_THREAD_LIBS_INIT})
set_property(TARGET hx-lang_parsing_tests PROPERTY CXX_STANDARD 17)


add_executable(hx-lang_vm_tests test/vm.cpp
  "${hx-lang_vm_files}")
target_compile_definitions(hx-lang_vm_tests PUBLIC -DH_LANG_TESTING)
target_link_libraries(hx-lang_vm_tests Catch2::Catch2 fmt::fmt-header-only stdc++fs ${CMAKE_THREAD_LIBS_INIT})
set_property(TARGET hx-lang_vm_tests PROPERTY CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/lib/Catch2/contrib/")

include(CTest)
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(hx-lang_parsing_tests)
ParseAndAddCatchTests(hx-lang_vm_tests)

## integration tests
add_test(NAME hx-lang_run_integration_test
  COMMAND bash -c "cd ${CMAKE_CURRENT_SOURCE_DIR}/test/integration/ ; ./run_all.sh"
)

# main program
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_executable(hx-lang src/main.cpp src/argument_parser.cpp "${hx-lang_files}")
target_link_libraries(hx-lang PRIVATE fmt::fmt-header-only stdc++fs ${CMAKE_THREAD_LIBS_INIT})
set_property(TARGET hx-lang PROPERTY CXX_STANDARD 17)

# virtual machine REPL
add_executable(hx-vm src/vm_main.cpp "${hx-lang_vm_files}")
target_link_libraries(hx-vm PRIVATE fmt::fmt-header-only stdc++fs ${CMAKE_THREAD_LIBS_INIT})
set_property(TARGET hx-vm PROPERTY CXX_STANDARD 17)

