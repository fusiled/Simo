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

#define BOOST_TEST_MODULE SimoTimePeriod
#include <glaze/glaze.hpp>
#include <sstream>
#include <unordered_set>

#include "Simo/Simo.h"
#include "support/BoostInclude.h"

BOOST_AUTO_TEST_CASE(TimeUnitConversions) {
  using Simo::Time;

  BOOST_CHECK_EQUAL(Time(1, Time::Unit::PS).to_picoseconds(), 1);
  BOOST_CHECK_EQUAL(Time(1, Time::Unit::NS).to_picoseconds(), 1'000);
  BOOST_CHECK_EQUAL(Time(1, Time::Unit::US).to_picoseconds(), 1'000'000);
  BOOST_CHECK_EQUAL(Time(1, Time::Unit::MS).to_picoseconds(), 1'000'000'000);
  BOOST_CHECK_EQUAL(Time(1, Time::Unit::S).to_picoseconds(), 1'000'000'000'000);
}

BOOST_AUTO_TEST_CASE(TimeArithmeticWithTimeOperands) {
  using Simo::Time;

  const Time lhs(30);
  const Time rhs(12);

  BOOST_CHECK_EQUAL((lhs + rhs).to_picoseconds(), 42);
  BOOST_CHECK_EQUAL((lhs - rhs).to_picoseconds(), 18);
  BOOST_CHECK_EQUAL((lhs * rhs).to_picoseconds(), 360);
  BOOST_CHECK_EQUAL((lhs / rhs).to_picoseconds(), 2);
  BOOST_CHECK_EQUAL((lhs % rhs).to_picoseconds(), 6);
}

BOOST_AUTO_TEST_CASE(TimeArithmeticWithIntegralOperands) {
  using Simo::Time;

  const Time base(42);

  BOOST_CHECK_EQUAL((base - 2).to_picoseconds(), 40);
  BOOST_CHECK_EQUAL((base * 3).to_picoseconds(), 126);
  BOOST_CHECK_EQUAL((base / 2).to_picoseconds(), 21);
  BOOST_CHECK_EQUAL((base % 5).to_picoseconds(), 2);
}

BOOST_AUTO_TEST_CASE(TimeComparisonAndEquality) {
  using Simo::Time;

  const Time t1(100);
  const Time t2(101);
  const Time t3(100);

  BOOST_CHECK(t1 < t2);
  BOOST_CHECK(t2 > t1);
  BOOST_CHECK(t1 <= t3);
  BOOST_CHECK(t1 >= t3);
  BOOST_CHECK(t1 == t3);
  BOOST_CHECK(!(t1 == t2));
}

BOOST_AUTO_TEST_CASE(TimeHashCanBeUsedInUnorderedSet) {
  using Simo::Time;

  std::unordered_set<Time> times;
  times.emplace(Time(10));
  times.emplace(Time(10));
  times.emplace(Time(20));

  BOOST_CHECK_EQUAL(times.size(), 2);
  BOOST_CHECK(times.contains(Time(10)));
  BOOST_CHECK(times.contains(Time(20)));
  BOOST_CHECK(!times.contains(Time(30)));
}

BOOST_AUTO_TEST_CASE(TimeStreamFormatting) {
  using Simo::Time;

  std::stringstream ss;
  ss << Time(42);

  BOOST_CHECK_EQUAL(ss.str(), "Time( 42 ps)");
}

BOOST_AUTO_TEST_CASE(TimeJsonSerializationAndParsing) {
  using Simo::Time;

  Time parsed;
  const auto parse_error = glz::read_json(parsed, R"({"time":7,"unit":"NS"})");
  BOOST_CHECK(!parse_error);
  BOOST_CHECK_EQUAL(parsed.to_picoseconds(), 7'000U);

  const auto serialized = glz::write_json(Time(5));
  BOOST_REQUIRE(serialized.has_value());
  BOOST_CHECK_EQUAL(*serialized, R"({"time":5,"unit":"PS"})");

  Time unchanged(99);
  const auto invalid_error =
      glz::read_json(unchanged, R"({"time":"bad","unit":"PS"})");
  BOOST_CHECK(invalid_error);
  BOOST_CHECK_EQUAL(unchanged.to_picoseconds(), 99U);
}
