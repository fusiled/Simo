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

# Does checks with ast-grep to avoid some pitfalls.
# Run this script from root of the repo to have sgconfig.yaml available
ast-grep scan include
success="$?"

if [ "$success" -ne 0 ]; then
    echo "ast-grep checks failed. Fix the issues above. If you are confident with auto-fixes, \
run 'ast-grep scan include --update-all' or 'ast-grep scan include --update-all' and verify your change."
    exit 1
else
    echo "ast-grep checks passed successfully."
fi

