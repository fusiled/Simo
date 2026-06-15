# Copyright 2026 Matteo Fusi and Contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#    http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

cmake_minimum_required(VERSION 3.31.0)
project(SimoTutorial VERSION 0.0.1)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# Look for Simo package
find_package(Simo REQUIRED)

# Create you collection of modules
add_library(FirstModuleCollection SHARED
        FirstModule.cc
)
# Hide symbols by default
set_target_properties(FirstModuleCollection PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN 1
)

# Use Simo::Simo_includes_only to only use the include files and avoid direct linking to Simo
target_link_libraries(FirstModuleCollection PRIVATE Simo::Simo_includes_only)
