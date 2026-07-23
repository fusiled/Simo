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
bool Parameters::check() const {
  return trie.all([](const Parameter::Parameter& p) { return p.validate(); });
}

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

InitializationStatus Module::initialize(Context& sim_ctx_v,
                                        const Parameters& parameters) {
  sim_ctx_ = &sim_ctx_v;
  name_ = parameters.name();
  return parameters.check()
             ? InitializationStatus::ok(this)
             : InitializationStatus(this, {"Parameter check failed"});
}

std::string_view Module::name() const { return name_; }

Context& Module::sim_ctx() const { return *sim_ctx_; }

Time Module::current_time() const { return sim_ctx().current_time(); }

void Module::record_statistics(Statistics::StatMapper& mapper) {
  statistics.visit([&mapper](auto& s) { mapper.add(s); });
}

Port* Module::get_port(const std::string_view name) {
  return ports.contains(std::string(name)) ? ports[std::string(name)].get()
                                           : nullptr;
}

std::vector<Module::PortWithFullName> Module::get_unconnected_ports(
    bool include_nested_components) const {
  std::vector<PortWithFullName> ret;
  for (const auto& p : ports) {
    if (!p.second->connected()) {
      ret.emplace_back(name_of_child(p.first), p.second.get());
    }
  }
  if (!include_nested_components) {
    return ret;
  }
  for (const auto& child : children) {
    const auto child_ret =
        child->get_unconnected_ports(include_nested_components);
    ret.insert(ret.end(), child_ret.begin(), child_ret.end());
  }
  return ret;
}

InitializationStatus Module::log_setup(const std::filesystem::path& out_file) {
  logger = {};
  return logger.initialize(out_file);
}

InitializationStatus Module::log_setup() { return log_setup("Simo.log"); }

void Module::log_enable(const bool new_value) { logger.enabled(new_value); }

void Module::log_level(const size_t level) { logger.log_level(level); }

void Module::log_level(const std::string_view level_name) {
  logger.log_level(level_name);
}

void Module::populate_default_log_levels() {
  logger.populate_default_log_levels();
}

}  // namespace Simo