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

#ifndef SIMO_MODULE_HH
#define SIMO_MODULE_HH

#include <string>

#include "Simo/parameter/ParameterTrie.h"
#include "Simo/port/Port.h"
#include "Simo/statistics/StatMapper.h"

namespace Simo {

class Context;

class SIMO_PUBLIC Parameters {
 public:
  virtual ~Parameters() = default;

  [[nodiscard]] std::string_view name() const;

  void name(std::string_view name);

  // TODO make it to return a list of errors
  [[nodiscard]] virtual bool check() const;

  template <typename T>
  [[maybe_unused]]
  Parameter::ParameterTyped<T>& set(const std::string& name, const T& value) {
    return trie.add<T>(name, value);
  }

  template <typename T>
  [[nodiscard]] Parameter::ParameterTyped<T>* get(
      const std::string& name) const {
    return trie.find<T>(name);
  }

  [[nodiscard]] std::optional<Parameters> get_subtree(
      const std::string& name) const;

 protected:
  Parameter::ParameterTrie trie;
  std::string name_;
};

class SIMO_PUBLIC Module {
 public:
  [[nodiscard]]
  virtual bool initialize(Context& sim_ctx_v, const Parameters& parameters);

  [[nodiscard]] std::string_view name() const;

  [[nodiscard]] Context& sim_ctx() const;

  virtual ~Module() = default;

  void record_statistics(Statistics::StatMapper& mapper);

  [[nodiscard]] Port* get_port(std::string_view);

  template <typename Stat>
  Stat* get_statistic(const std::string_view name) {
    return statistics.get<Stat>(name);
  }

 protected:
  template <typename T, typename... Args>
  T& create_statistic(Args... args) {
    T& s = statistics.emplace<T>(std::forward<Args>(args)...);
    return s;
  }

  template <typename T, typename... Args>
  T& create_port(const std::string_view name, Args... args) {
    auto ptr = std::make_unique<T>(std::forward<Args>(args)...);
    T& out_ref = *ptr;
    ports[std::string(name)] = std::move(ptr);
    return out_ref;
  }

  Statistics::StatStorage statistics;
  std::unordered_map<std::string, std::unique_ptr<Port>> ports;

 private:
  std::string name_;
  Context* sim_ctx_ = nullptr;
};
}  // namespace Simo

#endif  // SIMO_MODULE_HH