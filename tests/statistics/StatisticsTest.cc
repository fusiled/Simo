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
#include <Simo/core/Context.h>
#include <Simo/module/Module.h>
#include <Simo/module/core/Collector.h>
#include <Simo/module/core/CoreModules.h>
#include <Simo/parameter/Parameter.h>
#include <Simo/port/Port.h>
#include <Simo/statistics/Count.h>
#include <Simo/statistics/StatMapper.h>
#include <Simo/statistics/StatOutStream.h>

#include <cstdint>
#include <string>
#include <unordered_map>

#include "support/BoostInclude.h"

namespace Simo::Tests {
namespace {

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

  void increment_by(const int64_t value) { *sent_counter_ += value; }

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

class TestStatistic final : public Simo::Statistics::Statistic {
 public:
  TestStatistic() {}

  std::unique_ptr<Statistic> clone() const override {
    return std::make_unique<TestStatistic>();
  }

  void assign_from(const Statistic&) override {}

  glz::generic_u64 dump_representation() const override {
    return glz::generic_u64{};
  }

  [[nodiscard]] std::unique_ptr<Statistic> compute_delta(
      const Statistic&) const override {
    return std::make_unique<TestStatistic>();
  }

  ~TestStatistic() override = default;
};

BOOST_AUTO_TEST_CASE(count_compute_delta_different_type) {
  using Simo::Statistics::Count;

  Count lhs("lhs", 12);

  TestStatistic rhs;

  auto delta = lhs.compute_delta(rhs);
  BOOST_CHECK_EQUAL(delta.get(), nullptr);
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

  const glz::generic_u64::array_t expected = {
      sent.dump_representation(),
      received.dump_representation(),
  };
  auto expected_str = glz::write_yaml(expected);
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
    BOOST_FAIL("Unexpected statistic name: " + std::string(count->name()));
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
    BOOST_FAIL("Unexpected statistic name: " + std::string(count->name()));
  }
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

BOOST_AUTO_TEST_CASE(
    CoreModulesCollectionFactoryCreatesCollectorAndParameters) {
  const auto collection_with_lib = Simo::Modules::Core::loadCoreModules();
  const auto* collection = collection_with_lib.get_collection();
  BOOST_REQUIRE_NE(collection, nullptr);
  BOOST_CHECK_EQUAL(collection->name, "SimoCoreModules");
  BOOST_CHECK(collection->version ==
              Simo::Collections::SimoCollectionVersion(0, 0, 1));

  const auto* factory = collection->get_factory("collector");
  BOOST_REQUIRE_NE(factory, nullptr);
  BOOST_CHECK_EQUAL(factory->get_name(), "collector");

  auto collector = factory->get_module();
  auto params = factory->get_parameters();
  BOOST_REQUIRE_NE(collector, nullptr);
  BOOST_REQUIRE_NE(params, nullptr);

  auto start_time_value =
      glz::read_json<glz::generic_u64>(R"({"time":1,"unit":"NS"})");
  BOOST_REQUIRE(start_time_value.has_value());
  auto* start_time = params->get("start_time");
  BOOST_REQUIRE_NE(start_time, nullptr);
  auto start_parse_result = start_time->value_from_generic(*start_time_value);
  BOOST_REQUIRE(start_parse_result.has_value());
  BOOST_CHECK_EQUAL(start_parse_result.value(), start_time);

  auto end_time_value =
      glz::read_json<glz::generic_u64>(R"({"time":2,"unit":"NS"})");
  BOOST_REQUIRE(end_time_value.has_value());
  auto* end_time = params->get("end_time");
  BOOST_REQUIRE_NE(end_time, nullptr);
  BOOST_REQUIRE(end_time->value_from_generic(*end_time_value).has_value());

  auto invalid_time_value =
      glz::read_json<glz::generic_u64>(R"({"time":"bad","unit":"NS"})");
  BOOST_REQUIRE(invalid_time_value.has_value());
  BOOST_CHECK_EQUAL(
      end_time->value_from_generic(*invalid_time_value).has_value(), false);

  const auto subtree = params->get_subtree("");
  BOOST_REQUIRE(subtree.has_value());
  BOOST_CHECK_EQUAL(subtree->get<Simo::Time>("start_time")->value(),
                    Simo::Time(1, Simo::Time::Unit::NS));
}

BOOST_AUTO_TEST_CASE(CollectorParametersValidation) {
  using Simo::Time;
  using Simo::Modules::Core::Collector;

  Collector::Parameters params;
  BOOST_CHECK_EQUAL(params.check(), true);

  params.get<Time>("start_time")->value(Time(10));
  params.get<Time>("end_time")->value(Time(5));
  BOOST_CHECK_EQUAL(params.check(), false);

  params.get<Time>("end_time")->value(Time(20));
  BOOST_CHECK_EQUAL(params.check(), true);
}

BOOST_AUTO_TEST_CASE(CollectorCollectsMatchingModuleStatistics) {
  using Simo::Context;
  using Simo::Parameters;
  using Simo::Time;
  using Simo::Modules::Core::Collector;

  const auto output_path =
      std::filesystem::temp_directory_path() / "simo-collector-stats.json";
  std::filesystem::remove(output_path);

  Context sim_ctx;
  StatisticRecordingModule recorded_module;
  StatisticRecordingModule ignored_module;
  Collector collector;

  Parameters recorded_params;
  recorded_params.name("recorded_module");
  Parameters ignored_params;
  ignored_params.name("ignored_module");
  Collector::Parameters collector_params;
  collector_params.name("collector");
  collector_params.get<Time>("start_time")->value(Time(10));
  collector_params.get<Time>("end_time")->value(Time(20));
  collector_params.get<std::string>("module_match_regex")->value("recorded_.*");
  collector_params.get<std::string>("dump_path")->value(output_path.string());

  sim_ctx.add(recorded_module, recorded_params);
  sim_ctx.add(ignored_module, ignored_params);
  sim_ctx.add(collector, collector_params);

  const auto status = sim_ctx.initialize();
  BOOST_REQUIRE_EQUAL(status.success(), true);
  sim_ctx.run_at(Time(10));
  recorded_module.increment_by(3);
  ignored_module.increment_by(9);
  sim_ctx.run_at(Time(20));

  BOOST_REQUIRE(std::filesystem::exists(output_path));
  const auto contents = read_file_contents(output_path);
  BOOST_CHECK(contents.find("sent") != std::string::npos);
  BOOST_CHECK(contents.find("3") != std::string::npos);
  BOOST_CHECK(contents.find("9") == std::string::npos);

  std::filesystem::remove(output_path);
}

BOOST_AUTO_TEST_CASE(CollectorInitializeFailsWithInvalidParameters) {
  using Simo::Context;
  using Simo::Time;
  using Simo::Modules::Core::Collector;

  Context sim_ctx;
  Collector collector;
  Collector::Parameters params;
  params.get<Time>("start_time")->value(Time(10));
  params.get<Time>("end_time")->value(Time(5));

  BOOST_CHECK_EQUAL(collector.initialize(sim_ctx, params).success(), false);
}
}  // namespace Simo::Tests
