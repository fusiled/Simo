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

#ifndef SIMO_INITIALIZATIONSTATUS_H
#define SIMO_INITIALIZATIONSTATUS_H

#include <format>
#include <ostream>
#include <string>
#include <string_view>
#include <vector>

#include "Simo/compiler/Compiler.h"

namespace Simo {

class Module;

class SIMO_PUBLIC InitializationStatus {
 public:
  InitializationStatus(Module* module,
                       const std::vector<std::string>& error_list);

  explicit InitializationStatus(Module* module);

  static InitializationStatus ok(Module* module);

  void add_error(std::string_view new_error);

  [[nodiscard]] std::string to_string() const;

  template <typename... Args>
  InitializationStatus& emplace_sub_error(Args... args) {
    sub_errors.emplace_back(args...);
    success_ &= sub_errors.back().success();
    return sub_errors.back();
  }

  [[nodiscard]] bool success() const;

  explicit operator bool() const;

 protected:
  Module* module = nullptr;
  std::vector<std::string> error_list;
  std::vector<InitializationStatus> sub_errors;
  bool success_ = false;

  void append_to(std::string& out, std::size_t indent) const;
};

inline std::ostream& operator<<(std::ostream& os,
                                const InitializationStatus& status) {
  return os << status.to_string();
}

}  // namespace Simo

template <>
struct std::formatter<Simo::InitializationStatus, char>
    : std::formatter<std::string, char> {
  auto format(const Simo::InitializationStatus& status,
              std::format_context& ctx) const {
    return std::formatter<std::string, char>::format(status.to_string(), ctx);
  }
};

#endif  // SIMO_INITIALIZATIONSTATUS_H