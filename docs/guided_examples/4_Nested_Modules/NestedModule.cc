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

class NestedModule : public Simo::Module {
 public:
  Simo::InitializationStatus initialize(Simo::Context& ctx,
                                        const Simo::Parameters& p) override {
    if (const auto status = Module::initialize(ctx, p); !status.success()) {
      return status;
    }
    bool success_init = p.get<bool>("nested_module_success")->value();
    return success_init ? Simo::InitializationStatus::ok(this)
                        : Simo::InitializationStatus(
                              this, {"Nested module failed to initialize"});
  }
};

class RootModule : public Simo::Module {
 public:
  Simo::InitializationStatus initialize(Simo::Context& ctx,
                                        const Simo::Parameters& p) override {
    if (const auto status = Module::initialize(ctx, p); !status.success()) {
      return status;
    }
    nested_module = &create_child<NestedModule>();
    // Propagate params to nested component through a copy
    Simo::Parameters nested_param = p;
    nested_param.name(name_of_child("nested_module"));
    auto nested_module_success = nested_module->initialize(ctx, nested_param);
    return !nested_module_success.success()
               ? nested_module_success
               : Simo::InitializationStatus::ok(this);
  }

  NestedModule* nested_module = nullptr;
};

int main() {
  using Simo::Period;
  using Simo::Time;

  Simo::Context sim_ctx;

  RootModule root_1;
  Simo::Parameters params_1;
  params_1.name("root_1");
  params_1.set<bool>("nested_module_success", false);

  sim_ctx.add(root_1, params_1);

  RootModule root_2;
  Simo::Parameters params_2;
  params_2.name("root_2");
  params_2.set<bool>("nested_module_success", true);
  sim_ctx.add(root_2, params_2);

  const auto initialize_success = sim_ctx.initialize();
  std::cout << initialize_success << std::endl;
}