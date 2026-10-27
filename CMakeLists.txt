cmake_minimum_required(VERSION 3.20)
project(sm83 C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release)
endif()

# The Lean toolchain comes from elan, following lean-toolchain.
find_program(LAKE lake REQUIRED)
find_program(LEAN lean REQUIRED)
execute_process(
  COMMAND ${LEAN} --print-prefix
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE LEAN_PREFIX
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
cmake_path(SET LEAN_PREFIX NORMALIZE "${LEAN_PREFIX}")

# Libraries needed by Lean-compiled code, as the toolchain itself reports them.
# Only archives that the model references are pulled in.
execute_process(
  COMMAND ${LEAN_PREFIX}/bin/leanc --print-ldflags
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  OUTPUT_VARIABLE LEAN_LDFLAGS
  OUTPUT_STRIP_TRAILING_WHITESPACE
  COMMAND_ERROR_IS_FATAL ANY)
string(REGEX REPLACE "-L +" "-L" LEAN_LDFLAGS "${LEAN_LDFLAGS}")
separate_arguments(LEAN_LDFLAGS NATIVE_COMMAND "${LEAN_LDFLAGS}")
# Keep only linker inputs; leanc also prints compile flags.
list(FILTER LEAN_LDFLAGS INCLUDE REGEX "^(-L|-l|-Wl,|-pthread)")
# CMake drops repeated flags, which breaks leanc's two archive groups; merge
# them into one group around all the libraries.
if("-Wl,--start-group" IN_LIST LEAN_LDFLAGS)
  list(REMOVE_ITEM LEAN_LDFLAGS "-Wl,--start-group" "-Wl,--end-group")
  list(PREPEND LEAN_LDFLAGS "-Wl,--start-group")
  list(APPEND LEAN_LDFLAGS "-Wl,--end-group")
endif()

# The model, compiled by Lake to a static library.
set(SM83_MODEL_LIB ${CMAKE_SOURCE_DIR}/.lake/build/lib/${CMAKE_STATIC_LIBRARY_PREFIX}sm83_Sm83${CMAKE_STATIC_LIBRARY_SUFFIX})
file(GLOB_RECURSE SM83_LEAN_SOURCES CONFIGURE_DEPENDS ${CMAKE_SOURCE_DIR}/Sm83/*.lean)
add_custom_command(
  OUTPUT ${SM83_MODEL_LIB}
  COMMAND ${LAKE} build Sm83
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  DEPENDS ${SM83_LEAN_SOURCES} ${CMAKE_SOURCE_DIR}/Sm83.lean ${CMAKE_SOURCE_DIR}/lakefile.lean
          ${CMAKE_SOURCE_DIR}/lean-toolchain
  COMMENT "Building the Lean model with Lake"
  VERBATIM)
add_custom_target(sm83_model DEPENDS ${SM83_MODEL_LIB})

add_executable(sm83 c/main.c c/sst.c c/bench.c c/json.c)
add_dependencies(sm83 sm83_model)
target_include_directories(sm83 PRIVATE ${LEAN_PREFIX}/include)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(sm83 PRIVATE -Wall -Wextra)
endif()
target_link_libraries(sm83 PRIVATE ${SM83_MODEL_LIB} -L${LEAN_PREFIX}/lib ${LEAN_LDFLAGS})
