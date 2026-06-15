#! /usr/bin/env bash
#
# Copyright 2026 Matteo Fusi and Contributors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

SCRIPT_DIR=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" &>/dev/null && pwd)
BUILD_TYPE="Debug"

while IFS= read -r -d '' dir
do
  pushd "$dir" > /dev/null || exit
  echo "Building example in $dir"
  cmake -B ./build "-DCMAKE_BUILD_TYPE=$BUILD_TYPE"
  cmake --build ./build --config "$BUILD_TYPE" -j
  popd > /dev/null || exit
done <   <(find "$SCRIPT_DIR" -mindepth 1 -maxdepth 1 -type d -print0)