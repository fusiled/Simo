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

#define BOOST_TEST_MODULE Statistics
#include <Simo/module/Module.h>
#include <Simo/parameter/Parameter.h>
#include <Simo/port/Port.h>
#include <Simo/statistics/Count.h>
#include <Simo/statistics/StatMapper.h>
#include <Simo/statistics/StatOutStream.h>

#include <boost/test/unit_test.hpp>
#include <cstdint>
#include <string>
#include <unordered_map>

namespace {

class NamedParameter final : public Simo::Parameter::Parameter {
 public:
  NamedParameter() : Simo::Parameter::Parameter("named_parameter") {}

  [[nodiscard]] bool validate() const override { return true; }

  [[nodiscard]] std::unique_ptr<Simo::Parameter::Parameter> clone()
      const override {
    return std::make_unique<NamedParameter>();
  }

  [[nodiscard]] std::string_view stored_name() const { return name_; }
};

class NamedPort final : public Simo::Port {
 public:
  [[nodiscard]] bool connect(Simo::Port* other) override {
    return other != nullptr;
  }
};

class StatisticRecordingModule final : public Simo::Module {
 public:
  StatisticRecordingModule() {
    sent_counter_ = &create_statistic<Simo::Statistics::Count>("sent", 0);
  }

  void increment() { ++(*sent_counter_); }

 private:
  Simo::Statistics::Count* sent_counter_ = nullptr;
};

[[nodiscard]] std::string read_file_contents(
    const std::filesystem::path& path) {
  std::ifstream input(path);
  std::ostringstream contents;
  contents << input.rdbuf();
  return contents.str();
}

}  // namespace

BOOST_AUTO_TEST_CASE(count_constructors_and_basic_mutation) {
  const Simo::Statistics::Count default_constructed;
  BOOST_CHECK_EQUAL(default_constructed.name(), "");
  BOOST_CHECK_EQUAL(default_constructed.value(), 0);
  BOOST_CHECK_EQUAL(default_constructed(), 0);

  Simo::Statistics::Count stat("test");
  BOOST_CHECK_EQUAL(stat.name(), "test");
  BOOST_CHECK_EQUAL(stat.value(), 0);
  BOOST_CHECK_EQUAL(stat(), 0);

  ++stat;
  BOOST_CHECK_EQUAL(stat(), 1);
  --stat;
  BOOST_CHECK_EQUAL(stat(), 0);

  stat += 5;
  BOOST_CHECK_EQUAL(stat(), 5);
  stat -= 5;
  BOOST_CHECK_EQUAL(stat(), 0);
}

BOOST_AUTO_TEST_CASE(StatStorage) {
  using Simo::Statistics::Count;
  Simo::Statistics::StatStorage storage;
  constexpr int64_t val = 12345;
  auto& el = storage.emplace<Count>("test", val);

  BOOST_CHECK_EQUAL(el.name(), "test");
  BOOST_CHECK_EQUAL(el.value(), val);
}

BOOST_AUTO_TEST_CASE(StatMapper) {
  Simo::Statistics::StatMapper mapper;
  Simo::Statistics::Count stat("test", 12);
  mapper.add(stat);

  stat += 3;
  auto stats_v = mapper.compute_diff();

  class TestStatStreamAfterAssign {
   public:
    using Count = Simo::Statistics::Count;

    TestStatStreamAfterAssign& operator<<(const Count& c) {
      if (c.name() == "test") {
        BOOST_CHECK_EQUAL(c.value(), 0);
      }
      return *this;
    }
  };

  mapper.assign();
  TestStatStreamAfterAssign after_assign;
  for (const auto& s : mapper.compute_diff()) {
    const auto* count =
        boost::typeindex::runtime_cast<const Simo::Statistics::Count*>(s.get());
    BOOST_REQUIRE_NE(count, nullptr);
    after_assign << *count;
  }
}

BOOST_AUTO_TEST_CASE(count_prefix_and_postfix_operators) {
  using Simo::Statistics::Count;

  Count c("counter", 10);

  const Count before_post_inc = c++;
  BOOST_CHECK_EQUAL(before_post_inc.name(), "counter");
  BOOST_CHECK_EQUAL(before_post_inc.value(), 10);
  BOOST_CHECK_EQUAL(c.value(), 11);

  const Count before_post_dec = c--;
  BOOST_CHECK_EQUAL(before_post_dec.name(), "counter");
  BOOST_CHECK_EQUAL(before_post_dec.value(), 11);
  BOOST_CHECK_EQUAL(c.value(), 10);

  Count& pre_inc_ref = ++c;
  BOOST_CHECK_EQUAL(pre_inc_ref.value(), 11);
  BOOST_CHECK_EQUAL(c.value(), 11);

  Count& pre_dec_ref = --c;
  BOOST_CHECK_EQUAL(pre_dec_ref.value(), 10);
  BOOST_CHECK_EQUAL(c.value(), 10);
}

BOOST_AUTO_TEST_CASE(count_assignment_arithmetic_serialize_and_delta) {
  using Simo::Statistics::Count;

  Count lhs("lhs", 12);
  Count rhs("rhs", 3);

  rhs = lhs;
  BOOST_CHECK_EQUAL(rhs.name(), "lhs");
  BOOST_CHECK_EQUAL(rhs.value(), 12);

  const Count plus_res = lhs + 5;
  BOOST_CHECK_EQUAL(plus_res.name(), "lhs");
  BOOST_CHECK_EQUAL(plus_res.value(), 17);

  const Count minus_res = lhs - Count("dummy", 7);
  BOOST_CHECK_EQUAL(minus_res.name(), "lhs");
  BOOST_CHECK_EQUAL(minus_res.value(), 5);

  auto delta = lhs.compute_delta(Count("lhs", 9));
  BOOST_CHECK_EQUAL(delta->name(), "lhs");
  const auto* delta_ptr = boost::typeindex::runtime_cast<Count*>(delta.get());
  BOOST_REQUIRE_NE(delta_ptr, nullptr);
  BOOST_CHECK_EQUAL(delta_ptr->value(), 3);
}

BOOST_AUTO_TEST_CASE(stat_storage_get) {
  using Simo::Statistics::Count;
  using Simo::Statistics::StatStorage;

  StatStorage storage;

  auto& c = storage.emplace<Count>("count2", 22);

  auto* found = storage.get<Count>("count2");
  BOOST_CHECK_EQUAL(found, &c);

  auto* missing = storage.get<Count>("missing");
  BOOST_CHECK_EQUAL(missing, nullptr);
}

BOOST_AUTO_TEST_CASE(stat_out_stream_writes_streamed_statistics_and_resets) {
  using Simo::Statistics::Count;
  using Simo::Statistics::StatOutStream;

  const auto tmp_path = std::filesystem::temp_directory_path();

  StatOutStream output;
  BOOST_CHECK(output.output_path().empty());

  const std::filesystem::path configured_path =
      tmp_path / "configured-output.json";
  output.output_path(configured_path);
  BOOST_CHECK_EQUAL(output.output_path().string(), configured_path.string());

  Count sent("sent", 12);
  Count received("received", 4);

  StatOutStream& returned = (output << sent) << received;
  BOOST_CHECK_EQUAL(&returned, &output);

  output.generate();

  const glz::generic::array_t expected = {
      sent.to_json(),
      received.to_json(),
  };
  auto expected_str = glz::write_json(expected);
  BOOST_REQUIRE(expected_str);

  BOOST_REQUIRE(std::filesystem::exists(configured_path));
  BOOST_CHECK_EQUAL(read_file_contents(configured_path), *expected_str);

  output.reset();
  output.generate();
  BOOST_CHECK_EQUAL(read_file_contents(configured_path), "[]");
}

BOOST_AUTO_TEST_CASE(stat_mapper_compute_diff_and_assign) {
  using Simo::Statistics::Count;
  using Simo::Statistics::StatMapper;

  Count sent("sent", 10);
  Count recv("recv", 5);

  StatMapper mapper;
  mapper.add(sent);
  mapper.add(recv);

  sent += 7;
  recv -= 2;

  for (const auto& stat : mapper.compute_diff()) {
    const auto* count =
        boost::typeindex::runtime_cast<const Count*>(stat.get());
    BOOST_REQUIRE_NE(count, nullptr);
    if (count->name() == "sent") {
      BOOST_CHECK_EQUAL(count->value(), 7);
      continue;
    }
    if (count->name() == "recv") {
      BOOST_CHECK_EQUAL(count->value(), -2);
      continue;
    }
    BOOST_FAIL("Unexpected statistic name: " + count->name());
  }
  mapper.assign();

  sent += 3;
  recv += 9;

  for (const auto& stat : mapper.compute_diff()) {
    const auto* count =
        boost::typeindex::runtime_cast<const Count*>(stat.get());
    BOOST_REQUIRE_NE(count, nullptr);
    if (count->name() == "sent") {
      BOOST_CHECK_EQUAL(count->value(), 3);
      continue;
    }
    if (count->name() == "recv") {
      BOOST_CHECK_EQUAL(count->value(), 9);
      continue;
    }
    BOOST_FAIL("Unexpected statistic name: " + count->name());
  }
}

BOOST_AUTO_TEST_CASE(ParameterTypeAndNamedConstructorAreReachable) {
  NamedParameter param;

  BOOST_CHECK_EQUAL(param.stored_name(), "named_parameter");
  BOOST_CHECK(param.validate());
  const auto base_runtime_type = param.type();

  auto cloned = param.clone();
  BOOST_REQUIRE_NE(cloned, nullptr);
  BOOST_CHECK_EQUAL(cloned->type(), base_runtime_type);
}

BOOST_AUTO_TEST_CASE(PortNameGetterAndSetter) {
  NamedPort left;
  NamedPort right;

  left.name("left_port");
  right.name("right_port");

  BOOST_CHECK_EQUAL(left.name(), "left_port");
  BOOST_CHECK_EQUAL(right.name(), "right_port");
  BOOST_CHECK_EQUAL(left.connect(&right), true);
  BOOST_CHECK_EQUAL(left.connect(nullptr), false);
}

BOOST_AUTO_TEST_CASE(ModuleRecordStatisticsInvokesStorageVisitPath) {
  using Simo::Statistics::Count;

  StatisticRecordingModule module;
  module.increment();
  module.increment();

  Simo::Statistics::StatMapper mapper;
  module.record_statistics(mapper);

  auto delta_stats = mapper.compute_diff();
  BOOST_REQUIRE_EQUAL(delta_stats.size(), 1U);
  const auto* count_ptr =
      boost::typeindex::runtime_cast<const Count*>(delta_stats.front().get());
  BOOST_REQUIRE_NE(count_ptr, nullptr);
  BOOST_CHECK_EQUAL(count_ptr->name(), "sent");
  BOOST_CHECK_EQUAL(count_ptr->value(), 0);

  module.increment();
  delta_stats = mapper.compute_diff();
  BOOST_REQUIRE_EQUAL(delta_stats.size(), 1U);
  count_ptr =
      boost::typeindex::runtime_cast<const Count*>(delta_stats.front().get());
  BOOST_REQUIRE_NE(count_ptr, nullptr);
  BOOST_CHECK_EQUAL(count_ptr->value(), 1);
}
