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

#include "Simo/statistics/StatMapper.h"

#include "Simo/module/Module.h"

namespace Simo::Statistics {
ModuleStatMapper::ModuleStatMapper(Module& m) : module(m) {
  module.record_statistics(statMapper);
}

std::vector<std::unique_ptr<Statistic>> ModuleStatMapper::compute_diff() {
  auto stats_v = statMapper.compute_diff();
  for (auto& stat : stats_v) {
    stat->name(std::string(module.name()) + "/" + std::string(stat->name()));
  }
  return stats_v;
}

}  // namespace Simo::Statistics