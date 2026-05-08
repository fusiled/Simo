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

#ifndef SIMO_STATMAPPER_HH
#define SIMO_STATMAPPER_HH

#include <glaze/glaze.hpp>
#include <memory>
#include <vector>

#include "./Statistic.h"

namespace Simo::Statistics {

/// Keep reference of statistics and of a previous values of them
///
/// Usefull for dumping statistics during simulation
class StatMapper {
 public:
  void add(Statistic& stat) { storage.emplace_back(stat.clone(), &stat); }

  void assign() const {
    for (const auto& [fst, snd] : storage) {
      fst->assign_from(*snd);
    }
  }

  std::vector<std::unique_ptr<Statistic>> compute_diff() {
    std::vector<std::unique_ptr<Statistic>> out_v;
    for (const auto& [fst, snd] : storage) {
      out_v.emplace_back(snd->compute_delta(*fst));
    }
    return out_v;
  }

 private:
  std::vector<std::pair<std::unique_ptr<Statistic>, Statistic*>> storage;
};

class StatStorage {
 public:
  template <typename T, typename... Args>
  T& emplace(Args... args) {
    storage.emplace_back(std::make_unique<T>(args...));
    return static_cast<T&>(*storage.back().get());
  }

  [[nodiscard]] Statistic* get(const std::string_view name) const {
    for (const auto& p : storage) {
      if (p->name() == name) {
        return p.get();
      }
    }
    return nullptr;
  }

  template <typename T>
  [[nodiscard]] T* get(const std::string_view name) const {
    Statistic* stat = get(name);
    if (stat == nullptr) {
      return nullptr;
    }
    auto* ptr_cast = boost::typeindex::runtime_cast<T*>(stat);
    if (ptr_cast == nullptr) {
      return nullptr;
    }
    return ptr_cast;
  }

  template <typename Visitor>
  void visit(Visitor f) {
    for (const auto& p : storage) {
      f(*p);
    }
  }

 private:
  std::vector<std::unique_ptr<Statistic>> storage;
};

}  // namespace Simo::Statistics

#endif  // SIMO_STATMAPPER_HH
