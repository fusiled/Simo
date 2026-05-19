/*
 * Copyright 2026 Matteo Fusi and Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Simo/core/InitializationStatus.h"

namespace Simo {

InitializationStatus::InitializationStatus(
    Module* module, const std::vector<std::string>& error_list)
    : module(module), error_list(error_list), success_(error_list.empty()) {}

InitializationStatus::InitializationStatus(Module* module)
    : InitializationStatus(module, {}) {}

InitializationStatus InitializationStatus::ok(Module* module) {
  return InitializationStatus(module);
}

void InitializationStatus::add_error(std::string_view new_error) {
  error_list.emplace_back(new_error);
  success_ = false;
}

[[nodiscard]] bool InitializationStatus::success() const { return success_; }

InitializationStatus::operator bool() const { return success_; }

}  // namespace Simo