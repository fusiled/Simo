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

#ifndef SIMO_SIMULATIONCONTEXT_H
#define SIMO_SIMULATIONCONTEXT_H

#include <cstdint>
#include <expected>
#include <functional>
#include <string>
#include <vector>

#include "./Time.h"
#include "./internal/RadixHeap.h"
#include "Simo/compiler/Compiler.h"
#include "Simo/module/Module.h"
#include "Simo/parameter/ParameterTrie.h"

namespace Simo {

namespace Parameter {
class Parameter;
template <typename T>
class ParameterTyped;
}  // namespace Parameter

class Module;
class Parameters;

class Period;
using SimulationCallable = std::function<void(Time)>;

/// Manage Module initialization and schedule simulation events
///
/// The initial state of the simulation is INITIALIZATION. In this phase:
/// - Modules can be added
/// - Events can be scheduled with any time.
/// After the simulation starts (ie a call to a run*
/// method is performed), the state becomes RUNNING. In this state, events can
/// only be scheduled for a time in the future. The current time can be probed
/// with current_time() method. When no events are schedules, the state
/// transitions to STOPPED. New events can be scheduled even in this state
class SIMO_PUBLIC Context {
 public:
  enum struct State : uint8_t {
    INITIALIZATION,
    PORT_CONNECTION,
    RUNNING,
    STOPPED,
    ERROR
  };

  enum struct RunStatus : uint8_t {
    EVENTS_IN_QUEUE,
    STOPPED,
  };

  Context();

  ~Context() = default;

  /// Initialize the context and the registered modules
  ///
  /// This can be done before the first call to run method
  [[nodiscard]]
  InitializationStatus initialize();

  /// Run for the time expressed in time_delta
  ///
  /// If the simulation current_time is t and this function is called,
  /// current_time will be t + time_delta at the end of the function
  /// If the context is not initialized, the initialize method is going
  /// to be called before the execution
  std::expected<RunStatus, InitializationStatus> run(const Time& time_delta);

  /// Run at the time expressed in time_target
  ///
  /// Run the simulation untile time_target is reached.
  std::expected<RunStatus, InitializationStatus> run_at(const Time& final_time);

  /// Schedule event at time_target time
  void schedule_at(const Time& time_target, const SimulationCallable& callable);
  /// Schedule event at time_target time
  void schedule_at(const Time& time_target,
                   const std::function<void()>& callable);

  /// Schedule an event for time current_time + time_delta
  void schedule_in(const Time& time_delta, const SimulationCallable& callable);
  /// Schedule an event for time current_time + time_delta
  void schedule_in(const Time& time_delta,
                   const std::function<void()>& callable);

  // Methods to initialize the context

  /// Add parameter of specific type. Return reference to parameter
  template <typename T>
  [[maybe_unused]]
  Parameter::ParameterTyped<T>& add_parameter(const std::string& name,
                                              const T& value) {
    return parameters.add<T>(name, value);
  }

  /// Add module with a set of parameters to the context.
  ///
  /// Modules are going to be initialized during initialization
  void add(Module& module, Parameters&);

  /// Remove a modules from the context before initialization occurs
  void remove(const Module& module);

  template <typename Function>
  void foreach_module(Function f) const {
    for (const auto& module : modules | std::views::keys) {
      f(*module);
    }
  }

  [[nodiscard]] State get_state() const;
  [[nodiscard]] Time current_time() const;

 private:
  State state{State::INITIALIZATION};
  Time currentTime = Time::zero;
  std::unique_ptr<Internal::RadixHeap> nextTickHeap;
  std::unordered_map<Time, std::vector<SimulationCallable>> scheduledTasks;

  Parameter::ParameterTrie parameters;

  std::vector<std::pair<Module*, Parameters*>> modules;
};
}  // namespace Simo

#endif  // SIMO_SIMULATIONCONTEXT_H
