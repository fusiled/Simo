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

#ifndef SIMO_COLLECTOR_HH
#define SIMO_COLLECTOR_HH
#include <regex>
#include <vector>

#include "Simo/core/Context.h"
#include "Simo/core/Time.h"
#include "Simo/module/Module.h"
#include "Simo/statistics/StatOutStream.h"

namespace Simo::Modules::Core {
class Collector : public Module {
 public:
  class Parameters : public Simo::Parameters {
   public:
    Parameters() {
      trie.add_unset<Time>("start_time");
      trie.add_unset<Time>("end_time").validator([this](const Time& end) {
        const auto start_time_ptr = trie.find<Time>("start_time");
        return start_time_ptr != nullptr && end > start_time_ptr->value();
      });
      trie.add<std::string>("module_match_regex", ".*");
      trie.add<std::string>("dump_path", "statistics.yaml");
    }
  };

  InitializationStatus initialize(Context& sim_ctx_v,
                                  const Simo::Parameters& parameters) override {
    if (const auto status = Module::initialize(sim_ctx_v, parameters);
        !status.success()) {
      return status;
    }
    const auto start_time = parameters.get<Time>("start_time")->value();
    const auto end_time = parameters.get<Time>("end_time")->value();
    dump_path = parameters.get<std::string>("dump_path")->value();
    sim_ctx().schedule_at(start_time, [this] { open_window(); });
    sim_ctx().schedule_at(end_time, [this] { close_window(); });
    const auto module_regex =
        parameters.get<std::string>("module_match_regex")->value();
    sim_ctx().foreach_module([this, &module_regex](const Module& m) {
      if (std::regex_match(std::string(m.name()), std::regex(module_regex))) {
        add_module(const_cast<Module*>(&m));
      }
    });
    return InitializationStatus::ok(this);
  }

  void add_module(Module* module) { modules.push_back(module); }

  void collect() {
    if (!searched_statistics) {
      for (const auto& module : modules) {
        mapper.emplace_back(*module);
      }
      searched_statistics = true;
    }
  }

  void open_window() {
    collect();
    for (const auto& mod_mapper : mapper) {
      mod_mapper.assign();
    }
  }

  void close_window() {
    collect();
    auto out_stream = Statistics::StatOutStream();
    out_stream.output_path(dump_path);
    for (auto& mod_mapper : mapper) {
      for (const auto& stat_ptr : mod_mapper.compute_diff()) {
        out_stream << *stat_ptr;
      }
    }
    out_stream.generate();
  }

 protected:
  std::vector<Module*> modules;
  bool searched_statistics = false;
  std::vector<Statistics::ModuleStatMapper> mapper;
  std::filesystem::path dump_path;
};
}  // namespace Simo::Modules::Core

#endif  // SIMO_COLLECTOR_HH
