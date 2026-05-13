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

#ifndef SIMO_STATISTICS_COUNT_HH
#define SIMO_STATISTICS_COUNT_HH

#include <sstream>
#include <string>

#include "Simo/compiler/Compiler.h"
#include "Statistic.h"

namespace Simo::Statistics {
/// Simple statistic that count events
class SIMO_PUBLIC Count : public Statistic {
 public:
  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(Statistic)
  constexpr Count(const std::string& name, const int64_t initial_value)
      : Statistic(name), val(initial_value) {}

  constexpr Count(const Count& other) : Count(other.name_, other.val) {}

  explicit constexpr Count(const std::string& name) : Count(name, 0) {}

  constexpr Count() : Count("") {}

  Count& operator=(const Count& other);

  Count& operator+=(const int64_t& delta);

  Count& operator-=(const int64_t& delta);

  Count& operator++();

  Count operator++(int);

  Count& operator--();

  Count operator--(int);

  Count operator-(const Count& other) const;
  Count operator+(const int64_t& delta) const;
  [[nodiscard]] std::unique_ptr<Statistic> compute_delta(
      const Statistic& other) const override;
  void assign_from(const Statistic& other) override;

  int64_t operator()() const;

  [[nodiscard]] int64_t value() const;

  // std::string serialize() override;

  [[nodiscard]] std::unique_ptr<Statistic> clone() const override {
    return std::make_unique<Count>(*this);
  }

  [[nodiscard]] glz::generic_u64 dump_representation() const override;

 protected:
  int64_t val{0};
};
}  // namespace Simo::Statistics

#endif  // SIMO_STATISTICS_COUNT_HH
