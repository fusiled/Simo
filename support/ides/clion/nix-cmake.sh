#!/usr/bin/env sh
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

# Use the cmakeFlags set by Nix - this doesn't work with --build
#FLAGS=$(echo "$@" | grep -e '--build' > /dev/null || echo "$cmakeFlags")
#"$(dirname "$0")"/nix-run.sh cmake ${FLAGS:+"$FLAGS"} "$@"


SCRIPT_DIR=$(cd -- "$(dirname -- "$0")" && pwd)
PROJECT_DIR=$SCRIPT_DIR/../../../
set -x
nix develop "$PROJECT_DIR" --command "$SCRIPT_DIR/nix-run.sh" cmake "$@"
exit $?


