cmake_minimum_required(VERSION 3.5)

project(hx-lang LANGUAGES CXX VERSION 0.0.1)

set(hx-lang_files src/token.cpp
  src/backend/builder.cpp
  src/backend/node.cpp
  src/backend/pe.cpp
  src/parser.cpp
  src/tokenizer.cpp
  src/ast.cpp
  src/repl.cpp
  src/symbol.cpp
  src/source_range.cpp
  src/diagnostic.cpp
  src/stream_lookup.cpp
  src/program.cpp
  src/fixit_info.cpp
  src/compiler.cpp
  src/type_checking.cpp
  src/vm.cpp
  src/assembler.cpp
  )

set(hx-lang_vm_files src/vm.cpp
  src/backend/builder.cpp
  src/backend/node.cpp
  src/backend/pe.cpp
  src/repl.cpp
  src/parser.cpp
  src/tokenizer.cpp
  src/ast.cpp
  src/assembler.cpp
  src/program.cpp
  src/symbol.cpp
  src/token.cpp
  src/diagnostic.cpp
  src/stream_lookup.cpp
  src/source_range.cpp
  src/fixit_info.cpp
  src/type_checking.cpp
  )

# Dependencies
add_subdirectory(lib/hat-trie)
add_subdirectory(lib/robin-map)
find_package(nlohmann_json 3.2.0 REQUIRED)
find_package(fmt REQUIRED)
find_package(Threads REQUIRED)

set(ASMJIT_EMBED TRUE)
add_definitions(-DASMJIT_STATIC)

set(ASMJIT_DIR "./lib/asmjit/")
include("${ASMJIT_DIR}/CMakeLists.txt")

option(USE_WERROR_DBG "Treat warnings as errors when compiling hx-lang." ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS} -ggdb -Wpedantic -Wall -Wextra -Wno-unused-parameter -Wno-unused-variable -Wno-implicit-fallthrough -Wno-switch")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O2")

if(USE_WERROR_DBG)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Werror")
endif()

# tests
find_package(Catch2 REQUIRED)

add_executable(hx-lang_vm_tests test/vm.cpp
  "${hx-lang_vm_files}"
  "${ASMJIT_SRC}")
target_compile_definitions(hx-lang_vm_tests PUBLIC -DH_LANG_TESTING)
target_link_libraries(hx-lang_vm_tests Catch2::Catch2 fmt::fmt-header-only stdc++fs ${CMAKE_THREAD_LIBS_INIT} tsl::hat_trie tsl::robin_map nlohmann_json::nlohmann_json ${ASMJIT_DEPS})
set_property(TARGET hx-lang_vm_tests PROPERTY CXX_STANDARD 17)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/lib/Catch2/contrib/")

include(CTest)
include(ParseAndAddCatchTests)
ParseAndAddCatchTests(hx-lang_vm_tests)

## integration tests
add_test(NAME hx-lang_run_integration_test
  COMMAND bash -c "cd ${CMAKE_CURRENT_SOURCE_DIR}/test/integration/ ; ./run_all.sh"
)

# main program
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
add_executable(hx-lang src/main.cpp src/argument_parser.cpp "${hx-lang_files}" "${ASMJIT_SRC}")
target_link_libraries(hx-lang PRIVATE fmt::fmt-header-only stdc++fs ${CMAKE_THREAD_LIBS_INIT} tsl::hat_trie tsl::robin_map nlohmann_json::nlohmann_json ${ASMJIT_DEPS})
set_property(TARGET hx-lang PROPERTY CXX_STANDARD 17)

# virtual machine REPL
add_executable(hx-vm src/vm_main.cpp "${hx-lang_vm_files}" "${ASMJIT_SRC}")
target_link_libraries(hx-vm PRIVATE fmt::fmt-header-only stdc++fs ${CMAKE_THREAD_LIBS_INIT} tsl::hat_trie tsl::robin_map nlohmann_json::nlohmann_json ${ASMJIT_DEPS})
set_property(TARGET hx-vm PROPERTY CXX_STANDARD 17)

