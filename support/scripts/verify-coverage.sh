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


BUILD_FOLDER=${1:-.}
LINE_PERCENTAGE=${2:-70}

IGNORE_REGEX="Count.h|CLI11.hpp"

set -x

case "$(uname -s)" in
    Darwin*)
        export SO_SUFFIX="dylib"
        # Add macOS-specific commands here
        ;;
    Linux*)
        export SO_SUFFIX="so"
        # Add Linux-specific commands here
        ;;
    *)
        exit 1
        ;;
esac

SIMO_LIB_PATH="${BUILD_FOLDER}/libSimo.${SO_SUFFIX}"
PROFILE_DATA_PATH="${BUILD_FOLDER}/default.profdata"

profile_inputs=()
while IFS= read -r profile_input; do
    profile_inputs+=("${profile_input}")
done < <(find "${BUILD_FOLDER}" -maxdepth 1 -type f -name "*.profraw" | sort)
if (( ${#profile_inputs[@]} == 0 )); then
    echo "No .profraw files found in ${BUILD_FOLDER}"
    exit 1
fi

# Generate profile data
llvm-profdata merge "${profile_inputs[@]}" -o "${PROFILE_DATA_PATH}"

# Show report
llvm-cov report "${SIMO_LIB_PATH}" "-instr-profile=${PROFILE_DATA_PATH}" "--line-coverage-lt=${LINE_PERCENTAGE}" \
    --ignore-filename-regex="${IGNORE_REGEX}"

files=()
while IFS= read -r file; do
    files+=("${file}")
done < <(
          llvm-cov export "${SIMO_LIB_PATH}" "-instr-profile=${PROFILE_DATA_PATH}" \
          "--ignore-filename-regex=${IGNORE_REGEX}" \
          | jq -r "
              .data[].files[]
              | select(
                  .summary.regions.percent < ${LINE_PERCENTAGE}
                  and .summary.regions.count > 0
                )
              | .filename
            "
        )

set +x
if (( ${#files[@]} > 0  )); then
    echo "The following files do not meet the criteria of ${LINE_PERCENTAGE}% of regions covered:"
    for file in "${files[@]}"; do
        echo " - $file"
    done
    echo "To see line code coverage, run llvm-cov show \"${SIMO_LIB_PATH}\" \
\"-instr-profile=${PROFILE_DATA_PATH}\" \"-ignore-filename-regex=${IGNORE_REGEX}\" -show-regions \
<path-to-file>"
    exit 1
fi
