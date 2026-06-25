/*
 * Copyright 2026 Matteo Fusi and Contributors
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

#include <Simo/Simo.h>

#include <cassert>
#include <iostream>

class TestModule : public Simo::Module {
 public:
  Simo::InitializationStatus initialize(Simo::Context& ctx,
                                        const Simo::Parameters& p) override {
    if (const auto status = Module::initialize(ctx, p); !status.success()) {
      return status;
    }
    period = p.get<Simo::Time>("period")->value();
    counter = &create_statistic<Simo::Statistics::Count>("counter");
    sim_ctx().schedule_at(Simo::Time::zero, [this]() { update_state(); });
    return Simo::InitializationStatus::ok(this);
  }

  Simo::Time period = Simo::Time::zero;
  Simo::Statistics::Count* counter = nullptr;

 protected:
  void update_state() {
    (*counter) += 1;
    std::cout << "Counter update: " << counter->value() << std::endl;
    sim_ctx().schedule_in(period, [this]() { update_state(); });
  }
};

void event_function() { std::cout << "Function event" << std::endl; }

int main() {
  using Simo::Period;
  using Simo::Time;

  Simo::Context sim_ctx;

  Simo::Parameters params;
  params.name("test");
  params.set<Time>("period", Time(10));

  TestModule test_module;
  sim_ctx.add(test_module, params);

  sim_ctx.schedule_at(Simo::Time(15),
                      []() { std::cout << "Lambda event" << std::endl; });
  sim_ctx.schedule_at(Simo::Time(20), event_function);

  const auto initialize_success = sim_ctx.initialize();
  assert(initialize_success);
  std::cout << "Start simulation" << std::endl;
  sim_ctx.run_at(Time::one);
  assert(sim_ctx.current_time() == Time::one);
  assert(test_module.counter->value() == 1);
  sim_ctx.run_at(Time(11));
  assert(sim_ctx.current_time() == Time(11));
  assert(test_module.counter->value() == 2);
  sim_ctx.run_at(Time(20));
  assert(test_module.counter->value() == 3);
}