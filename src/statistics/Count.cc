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

#include "Simo/statistics/Count.h"

#include <stdexcept>

namespace Simo::Statistics {
Count& Count::operator=(const Count& other) {
  name_ = other.name_;
  val = other.val;
  return *this;
}

Count& Count::operator+=(const int64_t& delta) {
  val += delta;
  return *this;
}

Count& Count::operator-=(const int64_t& delta) {
  val -= delta;
  return *this;
}

Count& Count::operator++() {
  // prefix ++count
  ++val;
  return *this;
}

Count Count::operator++(int) {
  // postfix count++
  Count old(*this);
  ++val;
  return old;
}

Count& Count::operator--() {
  // prefix ++count
  --val;
  return *this;
}

Count Count::operator--(int) {
  // postfix count++
  Count old(*this);
  --val;
  return old;
}

Count Count::operator-(const Count& other) const {
  return {name_, val - other.val};
}

Count Count::operator+(const int64_t& delta) const {
  return {name_, val + delta};
}

std::unique_ptr<Statistic> Count::compute_delta(const Statistic& other) const {
  const auto* count = boost::typeindex::runtime_cast<const Count*>(&other);
  if (count == nullptr) {
    return {};
  }
  return std::make_unique<Count>(name_, val - count->val);
}

void Count::assign_from(const Statistic& other) {
  const auto* count = boost::typeindex::runtime_cast<const Count*>(&other);
  SIMO_ASSERT(count != nullptr);
  *this = *count;
}

int64_t Count::operator()() const { return val; }

int64_t Count::value() const { return val; }

/*
std::string Count::serialize() {
  std::stringstream ss;
  ss << "{ name: " << name_ << ", type: count, value: " << val << "}";
  return ss.str();
}*/

glz::generic Count::to_json() const { return {}; }

}  // namespace Simo::Statistics
