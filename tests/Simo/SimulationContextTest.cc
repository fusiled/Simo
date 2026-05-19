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

#include <Simo/Simo.h>

#include <boost/test/unit_test.hpp>
#include <cstddef>
#include <cstdint>
#include <string>

class InitTrackingModule final : public Simo::Module {
 public:
  Simo::InitializationStatus initialize(
      Simo::Context& sim_ctx_v, const Simo::Parameters& parameters) override {
    initialize_calls++;
    was_initialized = true;
    return Module::initialize(sim_ctx_v, parameters);
  }

  std::size_t initialize_calls{0};
  bool was_initialized{false};
};

class FailingInitModule final : public Simo::Module {
 public:
  Simo::InitializationStatus initialize(Simo::Context&,
                                        const Simo::Parameters&) override {
    initialize_calls++;
    return Simo::InitializationStatus(this, {"Initialization failed"});
  }

  std::size_t initialize_calls{0};
};

BOOST_AUTO_TEST_CASE(SimulationContextOneSchedule) {
  using Simo::Context;
  using Simo::Time;
  bool test = false;
  Context sim_ctx;
  constexpr uint64_t test_time = 1000;
  sim_ctx.schedule_at(Time(test_time), [&test, test_time](const Time t) {
    BOOST_CHECK_EQUAL(t.to_picoseconds(), test_time);
    test = true;
  });
  sim_ctx.run_at(Time(test_time));
  BOOST_CHECK_EQUAL(test, true);
}

BOOST_AUTO_TEST_CASE(SimulationContextNoRunUntilScheduledEvent) {
  using Simo::Context;
  using Simo::Time;
  bool test = false;
  Context sim_ctx;
  constexpr uint64_t test_time = 1000;
  sim_ctx.schedule_at(Time(test_time), [&test](Time) { test = true; });
  constexpr uint64_t run_until_time = 999;
  sim_ctx.run_at(Time(run_until_time));
  BOOST_CHECK_EQUAL(test, false);
}

BOOST_AUTO_TEST_CASE(
    SimulationContextScheduleMultipleEventsOutOfOrderBeforeInitialization) {
  using Simo::Context;
  using Simo::Time;
  bool first_call_executed = false;
  bool second_call_executed = false;
  Context sim_ctx;
  constexpr uint64_t test_time = 1000;
  sim_ctx.schedule_at(Time(test_time) * 2,
                      [&second_call_executed] { second_call_executed = true; });
  sim_ctx.schedule_at(Time(test_time), [&first_call_executed](const Time t) {
    BOOST_CHECK_EQUAL(t.to_picoseconds(), Time(test_time).to_picoseconds());
    first_call_executed = true;
  });
  constexpr uint64_t run_until_time = 1000;
  sim_ctx.run_at(Time(run_until_time));
  BOOST_CHECK_EQUAL(first_call_executed, true);
  BOOST_CHECK_EQUAL(second_call_executed, false);
}

BOOST_AUTO_TEST_CASE(ContextSchedulingRunAndStateTransitions) {
  using Simo::Context;
  using Simo::Time;

  Context sim_ctx;
  BOOST_CHECK(sim_ctx.get_state() == Context::State::INITIALIZATION);

  bool schedule_at_called = false;
  bool schedule_in_void_called = false;
  bool schedule_in_time_called = false;

  sim_ctx.schedule_at(Time(5), [&](const Time t) {
    BOOST_CHECK_EQUAL(t.to_picoseconds(), 5U);
    schedule_at_called = true;
  });

  sim_ctx.schedule_in(Time(3), [&]() {
    BOOST_CHECK_EQUAL(sim_ctx.current_time().to_picoseconds(), 3U);
    schedule_in_void_called = true;
  });

  sim_ctx.schedule_in(Time(4), [&](const Time t) {
    BOOST_CHECK_EQUAL(t.to_picoseconds(), 4U);
    schedule_in_time_called = true;
  });

  sim_ctx.run(Time(2));
  BOOST_CHECK_EQUAL(sim_ctx.current_time().to_picoseconds(), 2U);
  BOOST_CHECK_EQUAL(schedule_in_void_called, false);
  BOOST_CHECK_EQUAL(schedule_in_time_called, false);
  BOOST_CHECK_EQUAL(schedule_at_called, false);

  sim_ctx.run(Time(2));
  BOOST_CHECK_EQUAL(sim_ctx.current_time().to_picoseconds(), 4U);
  BOOST_CHECK_EQUAL(schedule_in_void_called, true);
  BOOST_CHECK_EQUAL(schedule_in_time_called, true);
  BOOST_CHECK_EQUAL(schedule_at_called, false);

  sim_ctx.run_at(Time(6));
  BOOST_CHECK_EQUAL(schedule_at_called, true);
  BOOST_CHECK(sim_ctx.get_state() == Context::State::STOPPED);
}

BOOST_AUTO_TEST_CASE(ContextModuleAddRemoveAffectsInitializationLifecycle) {
  using Simo::Context;
  using Simo::Parameters;
  using Simo::Time;

  Context sim_ctx;
  Parameters module_params;
  module_params.name("tracked_module");

  InitTrackingModule kept_module;
  InitTrackingModule removed_module;

  sim_ctx.add(kept_module, module_params);
  sim_ctx.add(removed_module, module_params);
  sim_ctx.remove(removed_module);

  sim_ctx.run_at(Time::one);

  BOOST_CHECK_EQUAL(kept_module.was_initialized, true);
  BOOST_CHECK_EQUAL(removed_module.was_initialized, false);
  BOOST_CHECK_EQUAL(kept_module.initialize_calls, 1U);
  BOOST_CHECK_EQUAL(removed_module.initialize_calls, 0U);

  BOOST_CHECK_EQUAL(kept_module.name(), "tracked_module");
  BOOST_CHECK_EQUAL(&kept_module.sim_ctx(), &sim_ctx);
}

BOOST_AUTO_TEST_CASE(
    ContextInitializeReturnsFalseWhenModuleInitializationFails) {
  using Simo::Context;
  using Simo::Parameters;

  Context sim_ctx;
  Parameters module_params;
  FailingInitModule failing_module;

  sim_ctx.add(failing_module, module_params);

  BOOST_CHECK_EQUAL(sim_ctx.initialize().success(), false);
  BOOST_CHECK_EQUAL(failing_module.initialize_calls, 1U);
  BOOST_CHECK(sim_ctx.get_state() == Context::State::ERROR);
}

BOOST_AUTO_TEST_CASE(ContextSchedulesRepeatedFutureTickWhileRunning) {
  using Simo::Context;
  using Simo::Time;

  Context sim_ctx;
  std::size_t calls_at_five = 0;

  sim_ctx.schedule_at(Time::one, [&] {
    sim_ctx.schedule_at(Time(5), [&](const Time t) {
      BOOST_CHECK_EQUAL(t.to_picoseconds(), 5U);
      ++calls_at_five;
    });
    sim_ctx.schedule_at(Time(5), [&] { ++calls_at_five; });
  });

  sim_ctx.run_at(Time(6));

  BOOST_CHECK_EQUAL(calls_at_five, 2U);
}

BOOST_AUTO_TEST_CASE(ContextAddParameterTemplateInstantiations) {
  using Simo::Context;
  using Simo::Time;

  Context sim_ctx;
  sim_ctx.add_parameter<uint32_t>("u32", 11U);
  sim_ctx.add_parameter<double>("double", 3.5);
  sim_ctx.add_parameter<bool>("bool", true);
  sim_ctx.add_parameter<std::string>("string", "hello");
  sim_ctx.add_parameter<Time>("time", Time(12));

  sim_ctx.run_at(Time::one);
  BOOST_CHECK(sim_ctx.get_state() == Context::State::STOPPED);
}

BOOST_AUTO_TEST_CASE(ModuleParametersGetSubtreeHitAndMiss) {
  using Simo::Parameters;

  Parameters params;
  params.name("root");
  params.set<uint32_t>("cluster/width", 64);

  const auto subtree = params.get_subtree("cluster");
  BOOST_REQUIRE(subtree.has_value());
  BOOST_CHECK_EQUAL(subtree->name(), "cluster");
  auto* width = subtree->get<uint32_t>("width");
  BOOST_REQUIRE_NE(width, nullptr);
  BOOST_CHECK_EQUAL(width->value(), 64U);

  auto missing = params.get_subtree("missing");
  BOOST_CHECK_EQUAL(missing.has_value(), false);
}
