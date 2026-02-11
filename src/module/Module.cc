// Copyright 2026 Matteo Fusi and Contributors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "Simo/module/Module.h"

#include "Simo/core/Context.h"

namespace Simo {
namespace Modules::Bidirectional {
class Port;
}

namespace Statistics {
class StatisticBase;
}

// TODO make it to return a list of errors
bool Parameters::check() const { return true; }

std::optional<Parameters> Parameters::get_subtree(
    const std::string& name) const {
  const auto* subtrie = trie.get_subtrie(name);
  if (subtrie == nullptr) {
    return std::nullopt;
  }
  auto ret = std::make_optional<Parameters>();
  ret->trie = *subtrie;
  ret->name(name);
  return ret;
}

std::string_view Parameters::name() const { return name_; }

void Parameters::name(const std::string_view name) { name_ = name; }

bool Module::initialize(Context& sim_ctx_v, const Parameters& parameters) {
  sim_ctx_ = &sim_ctx_v;
  name_ = parameters.name();
  return parameters.check();
}

std::string_view Module::name() const { return name_; }

Context& Module::sim_ctx() const { return *sim_ctx_; }

void Module::record_statistics(Statistics::StatMapper& mapper) {
  statistics.visit([&mapper](auto& s) { mapper.add(s); });
}

Port* Module::get_port(const std::string_view name) {
  return ports.contains(std::string(name)) ? ports[std::string(name)].get()
                                           : nullptr;
}

}  // namespace Simo