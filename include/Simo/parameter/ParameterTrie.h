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

#ifndef SIMO_PARAMETERTRIE_HH
#define SIMO_PARAMETERTRIE_HH

#include <Simo/compiler/Compiler.h>

#include <Simo/compiler/BoostTypeIndexRuntimeCast.hh>
#include <ranges>
#include <unordered_map>

#include "Parameter.h"

namespace Simo::Parameter {
class Parameter;
constexpr char PARAMETER_NODE_SEPARATOR = '/';

/// Store parameters in a trie structure
///
/// Parameters can be expressed in a trie-like system by using '/' command
/// in a path-like style
class SIMO_PUBLIC ParameterTrie {
  static constexpr std::pair<std::string_view, std::string_view>
  split_string_view(const std::string_view name) {
    const auto next_dot_pos = name.find(PARAMETER_NODE_SEPARATOR);
    if (next_dot_pos == std::string_view::npos) {
      return std::make_pair(name, std::string_view{});
    }
    return std::make_pair(name.substr(0, next_dot_pos),
                          name.substr(next_dot_pos + 1));
  }

 public:
  ParameterTrie() = default;

  ParameterTrie(const ParameterTrie& other) { *this = other; }

  ParameterTrie& operator=(const ParameterTrie& other) {
    if (other.value) {
      value = std::unique_ptr(other.value->clone());
    }
    for (const auto& [fst, snd] : other.children) {
      children[fst] = snd;
    }
    return *this;
  }

  template <typename T>
  [[nodiscard]]
  ParameterTyped<T>& add(const std::string_view name, const T& v) {
    if (name.empty()) {
      auto ptr = new ParameterTyped<T>(v);
      value = std::unique_ptr<Parameter>(ptr);
      return *ptr;
    }
    const auto [child_name, rest_of_name] = split_string_view(name);
    return children[std::string(child_name)].add(rest_of_name, v);
  }

  [[nodiscard]]
  Parameter* find(const std::string_view name) const {
    if (name.empty()) {
      return value.get();
    }
    const auto [child_name, rest_of_name] = split_string_view(name);
    if (!children.contains(std::string(child_name))) {
      return nullptr;
    }
    return children.at(std::string(child_name)).find(rest_of_name);
  }

  template <typename T>
  [[nodiscard]] ParameterTyped<T>* find(const std::string_view name) const {
    auto* const res = find(name);
    return res == nullptr
               ? nullptr
               : boost::typeindex::runtime_cast<ParameterTyped<T>*>(res);
  }

  [[nodiscard]] const ParameterTrie* get_subtrie(
      const std::string_view name) const {
    if (name.empty()) {
      return this;
    }
    const auto [child_name, rest_of_name] = split_string_view(name);
    if (!children.contains(std::string(child_name))) {
      return nullptr;
    }
    return children.at(std::string(child_name)).get_subtrie(rest_of_name);
  }

  /// Verify that predicate function is valid for all the parameters. Return
  /// false as soon as possible
  template <typename Function>
  [[nodiscard]]
  bool all(Function f) const {
    if (value != nullptr && !f(*value)) {
      return false;
    }
    for (const auto& snd : children | std::views::values) {
      if (!snd.all(f)) {
        return false;
      }
    }
    return true;
  }

  std::unique_ptr<Parameter> value;

 private:
  std::unordered_map<std::string, ParameterTrie> children;
};
}  // namespace Simo::Parameter

#endif  // SIMO_PARAMETERTRIE_HH