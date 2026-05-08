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
#include <Simo/module/Module.h>

#include <boost/test/unit_test.hpp>

class TestModule : public Simo::Module {
 public:
  bool initialize(Simo::Context& ctx, const Simo::Parameters& p) override {
    if (!Module::initialize(ctx, p)) {
      return false;
    }
    period = p.get<Simo::Time>("period")->value();
    counter = &create_statistic<Simo::Statistics::Count>("counter");
    sim_ctx().schedule_at(Simo::Time::zero, [this]() { update_state(); });
    return true;
  }

  Simo::Time period = Simo::Time::zero;
  Simo::Statistics::Count* counter = nullptr;

 protected:
  void update_state() {
    (*counter) += 1;
    sim_ctx().schedule_in(period, [this]() { update_state(); });
  }
};

class TestParameters : public Simo::Parameters {
 public:
  [[nodiscard]] bool check() const override {
    const auto* param = get<Simo::Time>("period");
    return param != nullptr && param->value() > Simo::Time(1);
  }
};

BOOST_AUTO_TEST_CASE(SimpleLoop) {
  Simo::Context sim_ctx;
  using Simo::Period;
  using Simo::Time;

  TestParameters params;
  params.name("test");
  params.set<Time>("period", Time(10));

  TestModule test_module;
  sim_ctx.add(test_module, params);

  const bool initialize_success = sim_ctx.initialize();
  BOOST_REQUIRE_EQUAL(initialize_success, true);
  sim_ctx.run_at(Time(1));
  BOOST_CHECK_EQUAL(sim_ctx.current_time(), Time(1));
  BOOST_CHECK_EQUAL(test_module.counter->value(), 1);
  sim_ctx.run_at(Time(11));
  BOOST_CHECK_EQUAL(sim_ctx.current_time(), Time(11));
  BOOST_CHECK_EQUAL(test_module.counter->value(), 2);
  sim_ctx.run_at(Time(20));
  BOOST_CHECK_EQUAL(test_module.counter->value(), 3);
}

BOOST_AUTO_TEST_CASE(InitializationFailure) {
  Simo::Context sim_ctx;
  using Simo::Period;
  using Simo::Time;

  TestParameters params;
  params.name("test");
  // TestParameters expects period to be at least 2, so the context
  // initialization will fail
  params.set<Time>("period", Time(1));

  TestModule test_module;
  sim_ctx.add(test_module, params);

  const bool initialize_success = sim_ctx.initialize();
  BOOST_REQUIRE_EQUAL(initialize_success, false);
}