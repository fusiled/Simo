#  Copyright 2026 Matteo Fusi and Contributors
#
#    Licensed under the Apache License, Version 2.0 (the "License");
#    you may not use this file except in compliance with the License.
#    You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing, software
#    distributed under the License is distributed on an "AS IS" BASIS,
#    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#    See the License for the specific language governing permissions and
#    limitations under the License.

cmake_minimum_required(VERSION 3.31.0)
project(Simo VERSION 0.0.1)
include(CPack)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PORTABLE_BUILD "Be verbose on build and turn warnings into errors for the sake of build portability" ON)
option(DEBUG_CMAKE "Enable debug output from CMake" OFF)
option(ENABLE_COVERAGE "Enable code coverage build" OFF)
option(ENABLE_RELEASE_LTO "Enable link-time optimization for release builds" ON)
option(ENABLE_RTTI "Enable RTTI to build libSimo" OFF)


if (ENABLE_COVERAGE)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Add the coverage flags for the compiler and linker
    add_compile_options("--coverage" "-Og" "-g")
    add_link_options("--coverage")
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-Og" "-g" "--coverage" "-fprofile-instr-generate" "-fcoverage-mapping")
    add_link_options("-fprofile-instr-generate" "--coverage" "-fcoverage-mapping")
  else()
    message(WARNING "Code coverage not supported for this compiler.")
  endif ()
endif ()

if (APPLE)
  ## Match recent GCC behavior and remove transitive includes.
  add_definitions(-D_LIBCPP_REMOVE_TRANSITIVE_INCLUDES=1)
endif ()

if (PORTABLE_BUILD)
  add_compile_options(-Werror -Wall -Wextra -pedantic)
endif ()


add_library(Simo SHARED
        src/core/Time.cc
        src/core/Context.cc
        src/core/RadixHeap.cc
        src/parameter/Parameter.cc
        src/parameter/ParameterTrie.cc
        src/collection/Collection.cc
        src/module/Module.cc
        src/statistics/Count.cc
)

IF (NOT "${ENABLE_RTTI}")
  target_compile_options(Simo PRIVATE "-fno-rtti")
endif ()

if ("${ENABLE_RELEASE_LTO}")
  set_target_properties(Simo PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif ()

set_target_properties(Simo PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_compile_options(Simo PRIVATE -fno-exceptions)

find_package(Boost REQUIRED COMPONENTS unit_test_framework program_options)
message("Boost version: ${Boost_VERSION}")

find_package(glaze REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE glaze::glaze)


if (DEBUG_CMAKE)
  message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
endif ()

target_include_directories(Simo SYSTEM PUBLIC ${Boost_INCLUDE_DIRS})

set_target_properties(Simo PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
)

target_include_directories(Simo
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_include_directories(Simo PRIVATE src)

install(TARGETS Simo)
install(DIRECTORY include/
        DESTINATION include
)


## Unit tests
enable_testing()
function(create_unit_test_executable name)
  add_executable("${name}")
  target_include_directories("${name}" PRIVATE include)
  target_include_directories("${name}" PRIVATE src)
  target_link_libraries("${name}" PRIVATE Boost::unit_test_framework)
  add_test("${name}" "${name}")
  if (ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_tests_properties("${name}" PROPERTIES
            ENVIRONMENT "LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/${name}.profraw"
    )
  endif ()
  IF (NOT "${ENABLE_RTTI}")
    target_compile_options("${name}" PRIVATE "-fno-rtti")
  endif ()
endfunction()

# General unit tests for the library
create_unit_test_executable(test_Simo)
target_sources(test_Simo PRIVATE
        tests/Simo/UnitTestMain.cc
        tests/Simo/SimulationContextTest.cc
        tests/Simo/MainLoopTests.cc
)
target_link_libraries(test_Simo PRIVATE Simo)


# Unit tests for internal logic
# Try to avoid linking against Simo lib as much as possible to speed up compilation.
create_unit_test_executable(test_RadixHeap)
target_sources(test_RadixHeap PRIVATE
        tests/core/RadixHeap/RadixHeapTest.cc
        src/core/RadixHeap.cc
)

create_unit_test_executable(test_TimePeriod)
target_sources(test_TimePeriod PRIVATE
        tests/core/TimePeriod/TimePeriodTest.cc
        src/core/Time.cc
)

create_unit_test_executable(test_Parameter)
target_sources(test_Parameter PRIVATE
        tests/parameter/ParameterTest.cc
)

create_unit_test_executable(test_Statistics)
target_sources(test_Statistics PRIVATE
        tests/statistics/StatisticsTest.cc
)
target_link_libraries(test_Statistics PRIVATE Simo)

add_library(SimoTestPingPongCollection SHARED
        tests/collection/PingPongCollection.cc
)
IF (NOT "${ENABLE_RTTI}")
  target_compile_options(SimoTestPingPongCollection PRIVATE "-fno-rtti")
endif ()
target_compile_options(SimoTestPingPongCollection PRIVATE -fno-exceptions)
# Compile only with includes. Do not link against libraries
target_link_libraries(SimoTestPingPongCollection PRIVATE $<COMPILE_ONLY:Simo>)
# Avoid linking against Simo
if (APPLE)
  target_link_options(SimoTestPingPongCollection PRIVATE
            "LINKER:-undefined,dynamic_lookup"
    )
else ()
  target_link_options(SimoTestPingPongCollection PRIVATE
            "LINKER:--allow-shlib-undefined"
    )
endif ()

# Make this custom command to trigger re-build for targets that uses SimoTestPingPongCollection libraries indirectly (like
# test_Collection)
add_custom_command(
        OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/SimoTestPingPongCollection_is_built"
        COMMAND ${CMAKE_COMMAND} -E touch ${CMAKE_CURRENT_BINARY_DIR}/SimoTestPingPongCollection_is_built
        DEPENDS SimoTestPingPongCollection
)
add_custom_target(SimoTestPingPongCollection_stamp
        DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/SimoTestPingPongCollection_is_built"
)

create_unit_test_executable(test_Collection)
target_sources(test_Collection PRIVATE
        tests/collection/CollectionTest.cc
)
target_link_libraries(test_Collection PRIVATE Simo)
add_dependencies(test_Collection SimoTestPingPongCollection_stamp)

set_source_files_properties(tests/collection/CollectionTest.cc PROPERTIES
        OBJECT_DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/SimoTestPingPongCollection_is_built
)
