/*
 * Copyright $YEAR $USER_NAME and Contributors
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

#ifndef SIMO_STATISTIC_HH
#define SIMO_STATISTIC_HH

#include <Simo/compiler/BoostTypeIndexRuntimeCast.h>

#include <glaze/glaze.hpp>

namespace Simo::Statistics {
/// Base class for all statistics
class Statistic {
 public:
  virtual ~Statistic() = default;
  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS()

  Statistic() = default;

  /// Create a new copy of this statistic
  [[nodiscard]] virtual std::unique_ptr<Statistic> clone() const = 0;

  /// Make difference of this statistic with other and return the different
  /// as a new instance
  [[nodiscard]] virtual std::unique_ptr<Statistic> compute_delta(
      const Statistic& other) const = 0;

  virtual void assign_from(const Statistic& other) = 0;

  [[nodiscard]] std::string_view name() const { return name_; }
  void name(std::string_view new_name) { name_ = new_name; }

  [[nodiscard]] virtual glz::generic_u64 dump_representation() const = 0;

 protected:
  std::string name_;

  explicit Statistic(const std::string_view name) : name_(name) {}
};
}  // namespace Simo::Statistics

#endif  // SIMO_STATISTIC_HH
