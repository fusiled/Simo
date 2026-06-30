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

# GCC build struggles on mac and it requires to enforce this variable to empty
# to successfully link agains libraries.
# This option is valid only on Apple architectures
set(CMAKE_OSX_ARCHITECTURES "" CACHE STRING "")

cmake_minimum_required(VERSION 3.31.0)
project(Simo VERSION 0.0.1)
include(CPack)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(PORTABLE_BUILD "Be verbose on build and turn warnings into errors for the sake of build portability" ON)
option(DEBUG_CMAKE "Enable debug output from CMake" OFF)
option(ENABLE_COVERAGE "Enable code coverage build" OFF)
option(ENABLE_RELEASE_LTO "Enable link-time optimization for release builds" ON)
option(ENABLE_DOCUMENTATION "Build Documentation" OFF)


add_compile_options("-fno-rtti")

if (ENABLE_COVERAGE)
  if (CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # Add the coverage flags for the compiler and linker
    add_compile_options("--coverage" "-Og" "-g")
    add_link_options("--coverage")
  elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    add_compile_options("-Og" "-g" "-fprofile-instr-generate" "-fcoverage-mapping")
    add_link_options("-fprofile-instr-generate" "-fcoverage-mapping")
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
        src/module/core/CoreModules.cc
        src/statistics/StatMapper.cc
        src/core/InitializationStatus.cc
        src/core/Log.cc
)

if ("${ENABLE_RELEASE_LTO}")
  set_target_properties(Simo PROPERTIES INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE)
endif ()

set_target_properties(Simo PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_compile_options(Simo PRIVATE -fno-exceptions)
target_compile_options(Simo PUBLIC -fno-rtti)

find_package(Boost REQUIRED)
message("Boost version: ${Boost_VERSION}")

find_package(glaze REQUIRED)
target_link_libraries(${PROJECT_NAME} PRIVATE Boost::boost glaze::glaze)


if (DEBUG_CMAKE)
  message(STATUS "Boost include dirs: ${Boost_INCLUDE_DIRS}")
endif ()

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

# Simo_includes_only is a header-only target
add_library(Simo_includes_only INTERFACE)
target_include_directories(Simo_includes_only
        INTERFACE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)
target_link_libraries(Simo_includes_only INTERFACE glaze::glaze)
target_compile_options(Simo_includes_only INTERFACE -fno-rtti)
# Avoid linking against Simo
if (APPLE)
  target_link_options(Simo_includes_only INTERFACE
          "LINKER:-undefined,dynamic_lookup"
  )
else()
  target_link_options(Simo_includes_only INTERFACE
          "LINKER:--allow-shlib-undefined"
  )
endif()


add_executable(SimoSim src/SimoSim/SimoSim.cc)
target_link_libraries(SimoSim PRIVATE Simo)

install(TARGETS Simo SimoSim Simo_includes_only
  EXPORT SimoTargets
)
install(DIRECTORY include/
        DESTINATION include
)
install(EXPORT SimoTargets
        FILE SimoTargets.cmake
        NAMESPACE Simo::
        DESTINATION lib/cmake/Simo
)

# Generate the Package Configuration
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        "cmake/Simo/SimoConfigVersion.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)
install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/support/cmake/SimoConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/cmake/Simo/SimoConfigVersion.cmake"
        DESTINATION lib/cmake/Simo)



## Unit tests
enable_testing()
function(create_unit_test_executable name)
  add_executable("${name}")
  target_include_directories("${name}" PRIVATE include src tests)
  target_link_libraries("${name}" PRIVATE Boost::boost)
  add_test("${name}" "${name}")
  if (ENABLE_COVERAGE AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set_tests_properties("${name}" PROPERTIES
            ENVIRONMENT "LLVM_PROFILE_FILE=${CMAKE_BINARY_DIR}/${name}.profraw"
    )
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

create_unit_test_executable(test_Module)
target_sources(test_Module PRIVATE
        tests/module/ModuleTest.cc
)
target_link_libraries(test_Module PRIVATE Simo)

add_library(SimoTestPingPongCollection SHARED
        tests/collection/PingPongCollection.cc
)
set_target_properties(SimoTestPingPongCollection PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
)
target_compile_options(SimoTestPingPongCollection PRIVATE -fno-exceptions)
# Compile only with includes. Do not link against libraries
target_link_libraries(SimoTestPingPongCollection PRIVATE Simo_includes_only)


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


if (ENABLE_DOCUMENTATION)
  find_package(Doxygen REQUIRED)

  set(DOCS_BUILD_DIR ${CMAKE_BINARY_DIR}/docs)

  set(DOXYGEN_FILE_PATTERNS "*.h" "*.cc" "*.md")
  set(DOXYGEN_GENERATE_HTML YES)
  set(DOXYGEN_BUILTIN_STL_SUPPORT YES)
  set(DOXYGEN_OUTPUT_DIRECTORY ${DOCS_BUILD_DIR}/doxygen)
  set(DOXYGEN_USE_MDFILE_AS_MAINPAGE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

  doxygen_add_docs(
          doxygen_docs
          include src docs tests "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
  )
endif()
