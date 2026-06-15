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

#include <cmath>
#include <cstdint>
#include <functional>
#include <glaze/core/common.hpp>
#include <glaze/core/context.hpp>
#include <glaze/core/custom.hpp>
#include <glaze/json/generic.hpp>
#include <limits>
#include <ostream>
#include <string>
#include <string_view>
#include <type_traits>

#include "../compiler/Compiler.h"
#include "glaze/api/impl.hpp"

namespace Simo {
/// Express time in simulation. The minimum time that can be expressed in
/// pico-seconds
///
/// The underlying representation is uint64_t, so all the operations are
/// performed on this data type
class SIMO_PUBLIC Time final {
 public:
  enum struct Unit : std::uint8_t { PS, NS, US, MS, S };

  explicit constexpr Time(const uint64_t time, const Unit unit) {
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

  explicit constexpr Time(const uint64_t ps_time) : Time(ps_time, Unit::PS) {}

  constexpr Time() : Time(0) {}

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

  friend struct glz::meta<Time>;
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

namespace glz {
using Simo::Time;

struct TimeValue {
  std::uint64_t time;
  Time::Unit unit;
};

template <unsigned int Format>
struct from<Format, Time> {
  template <auto Opts>
  static void op(Time& value, is_context auto&& ctx, auto&& it, auto&& end) {
    TimeValue tmp{};
    parse<Format>::template op<Opts>(tmp, ctx, it, end);

    if (ctx.error != error_code::none) {
      return;
    }

    value = Time{tmp.time, tmp.unit};
  }
};

template <unsigned int Format>
struct to<Format, Time> {
  template <auto Opts>
  static void op(const Time& value, is_context auto&& ctx, auto&& b,
                 auto&& ix) noexcept {
    TimeValue tmp{.time = value.to_picoseconds(), .unit = Time::Unit::PS};
    serialize<Format>::template op<Opts>(tmp, ctx, b, ix);
  }
};

}  // namespace glz

template <>
struct std::formatter<Simo::Time> : std::formatter<std::string> {
  auto format(Simo::Time t, format_context& ctx) const {
    return formatter<string>::format(std::format("{} ps", t.to_picoseconds()),
                                     ctx);
  }
};

#endif  // SIMO_TIME_HH
