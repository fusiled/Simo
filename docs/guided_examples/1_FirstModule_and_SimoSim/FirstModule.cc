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

// Note 0: this is the general include for Simo
#include <Simo/Simo.h>

namespace Simo::Examples {

/// Log periodically a message and count the number of time the log took place
class SIMO_PUBLIC FirstModule : public Module {
 public:
  // Note 1
  // Module initialization takes place here
  InitializationStatus initialize(Context& sim_ctx_p,
                                  const Parameters& parameters) override {
    // Call initialization of subclass
    if (const auto status = Module::initialize(sim_ctx_p, parameters);
        !status.success()) {
      // Fail if something bad happened
      return status;
    }
    // Initialize variables from parameters
    period = parameters.get<Time>("period")->value();

    // Allocate statistics
    num_events = &create_statistic<Statistics::Count>("num_events");

    // Setup logging
    if (const auto status = log_setup("tutorial_module.log");
        !status.success()) {
      return status;
    }
    populate_default_log_levels();
    log_level(parameters.get<std::string>("log_level")->value());

    // Start scheduling events
    // schedule_at schedule at the specified time
    sim_ctx().schedule_at(Time::zero, [this]() { log_event(); });
    return InitializationStatus::ok(this);
  }

  void log_event() {
    // schedule_in schedule and event at current_time + period
    ++(*num_events);
    log(Log::LogLevel::INFO, [this]() {
      return std::format("{}, Hello from TestModule!", num_events->value());
    });
    sim_ctx().schedule_in(period, [this]() { log_event(); });
  }

 protected:
  // Remember the period to use to schedule events
  Time period;
  // Pointer to a statistic of the class
  Statistics::Count* num_events = nullptr;
};

/// Parameters for TestModule
class SIMO_PUBLIC FirstModuleParameters : public Parameters {
 public:
  // Note 2
  // Parameters describe inputs of a Module.
  // Parameters with a default value are added with `trie.add<>`
  // and without a default with `trie.add_unset<>`
  FirstModuleParameters() {
    // validator method is used to verify if a parameter is valid
    trie.add_unset<Time>("period").validator(
        [](const auto& t) { return t > Time::one; });
    trie.add<std::string>("log_level", "WARNING");
  }
};

}  // namespace Simo::Examples

// Note 3 : SimoSim can dynamically load components through collections with
// the logic below.
// A collection is a list of factories.
// A factory tells how to instantiate a Module and an associated parameter
extern "C" {
constexpr Simo::Collections::SimoCollectionVersion VERSION{
    .major = 0, .minor = 0, .patch = 1};

constexpr Simo::Collections::Factory TUTORIAL_FACTORY{
    .name = "TestModule",
    .get_module_function =
        [] {
          return static_cast<Simo::Module*>(new Simo::Examples::FirstModule());
        },
    .get_parameters_function =
        [] {
          return static_cast<Simo::Parameters*>(
              new Simo::Examples::FirstModuleParameters());
        }};

const char* collection_name = "TestCollection";
constexpr std::array FACTORY_LIST{TUTORIAL_FACTORY};
static Simo::Collections::SimoCollection collection{
    .name = collection_name,
    .version = VERSION,
    .factory_list = FACTORY_LIST.data(),
    .factory_list_size = FACTORY_LIST.size(),
};

// This exposes the collection
SIMO_PUBLIC const Simo::Collections::SimoCollection* simo_get_collection() {
  return &collection;
}
}
