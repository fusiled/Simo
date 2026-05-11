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

#ifndef SIMO_TIME_HH
#define SIMO_TIME_HH

#include <cstdint>
#include <functional>
#include <ostream>
#include <type_traits>
#include <glaze/core/common.hpp>
#include <glaze/core/custom.hpp>

#include "../compiler/Compiler.h"



namespace Simo {
/// Express time in simulation. The minimum time that can be expressed in
/// pico-seconds
///
/// The underlying representation is uint64_t, so all the operations are
/// performed on this data type
class SIMO_PUBLIC Time final {
 public:
  enum struct Unit : std::uint8_t { PS, NS, US, MS, S };

  explicit constexpr Time(const uint64_t time, const Unit unit = Unit::PS) {
    switch (unit) {
      case Unit::PS:
        picoseconds = time;
        return;
      case Unit::NS:
        picoseconds = time * 1'000;
        return;
      case Unit::US:
        picoseconds = time * 1'000'000;
        return;
      case Unit::MS:
        picoseconds = time * 1'000'000'000;
        return;
      case Unit::S:
        picoseconds = time * 1'000'000'000'000;
        return;
    }
  }

  constexpr Time():Time(0) {}

  constexpr Time operator+(const Time& t) const {
    return Time(picoseconds + t.picoseconds);
  }

  constexpr Time operator-(const Time& t) const {
    return Time(picoseconds - t.picoseconds);
  }

  constexpr Time operator-(const uint64_t t) const {
    return Time(picoseconds - t);
  }

  constexpr Time operator*(const Time& t) const {
    return Time(picoseconds * t.picoseconds);
  }

  constexpr Time operator*(const uint64_t t) const {
    return Time(picoseconds * t);
  }

  constexpr Time operator/(const Time& t) const {
    return Time(picoseconds / t.picoseconds);
  }

  constexpr Time operator/(const uint64_t t) const {
    return Time(picoseconds / t);
  }

  [[nodiscard]]
  constexpr Time operator%(const Time& time) const {
    return Time(picoseconds % time.picoseconds);
  }

  [[nodiscard]]
  constexpr Time operator%(const uint64_t time) const {
    return Time(picoseconds % time);
  }

  constexpr bool operator==(const Time& t) const {
    return picoseconds == t.picoseconds;
  }

  [[nodiscard]] uint64_t to_picoseconds() const { return picoseconds; }

  auto operator<=>(const Time& time) const {
    return picoseconds <=> time.picoseconds;
  }

  friend std::ostream& operator<<(std::ostream& out, const Time& e);

  static Time zero;
  static Time one;

 private:
  std::uint64_t picoseconds;
};
}  // namespace Simo

template <>
struct std::hash<Simo::Time> {
  std::size_t operator()(Simo::Time const& t) const noexcept {
    return std::hash<uint64_t>{}(t.to_picoseconds());
  }
};

template <>
struct glz::meta<Simo::Time::Unit> {
  using enum Simo::Time::Unit;
  static constexpr auto value = enumerate(PS, NS, US, MS, S);
};

// Defines the mapping to JSON
template <>
struct glz::meta<Simo::Time> {
  static constexpr auto value = glz::custom<
      [](Simo::Time& t, std::uint64_t ps) { t = Simo::Time(ps); },
      [](const Simo::Time& t) -> std::uint64_t { return t.to_picoseconds(); }>;
};

#endif  // SIMO_TIME_HH