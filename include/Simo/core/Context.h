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

enum struct ParameterAdditionStatus : uint8_t {
  PARAMETER_NEW,
  PARAMETER_OVERWRITTEN,
};

enum struct ParameterAdditionError : uint8_t {
  INVALID_TYPE,
  SIMULATION_ALREADY_INITIALIZED,
};

/// Manages scheduled events of the simulation
///
/// The initial state of the simulation is INITIALIZATION, in which events can
/// be scheduled with any time. After the simulation starts (ie a call to a run*
/// method is performed), the state becomes RUNNING. In this state, events can
/// only be scheduled for a time in the future. The current time can be probed
/// with current_time() method.
class SIMO_PUBLIC Context {
 public:
  enum struct State : uint8_t { INITIALIZATION, RUNNING, STOPPED };

  Context();

  ~Context() {}

  /// Initialize the context and the registered modules
  [[nodiscard]]
  bool initialize();

  /// Run for the time expressed in time_delta
  ///
  /// If the simulation current_time is t and this function is called,
  /// current_time will be t + time_delta at the end of the function
  void run(const Time& time_delta);

  /// Run at the time expressed in time_target
  ///
  /// Run the simulation untile time_target is reached.
  void run_at(const Time& final_time);

  void schedule_at(const Time& time_target, const SimulationCallable& callable);
  void schedule_at(const Time& time_target,
                   const std::function<void()>& callable);

  /// Schedule an event for time current_time + time_delta
  void schedule_in(const Time& time_delta, const SimulationCallable& callable);
  void schedule_in(const Time& time_delta,
                   const std::function<void()>& callable);

  // Methods to initialize the context
  template <typename T>
  [[maybe_unused]]
  Parameter::ParameterTyped<T>& add_parameter(const std::string& name,
                                              const T& value) {
    return parameters.add<T>(name, value);
  }
  void add(Module& module, Parameters&);
  void remove(const Module& module);

  [[nodiscard]] State get_state() const;
  [[nodiscard]] Time current_time() const;
  [[nodiscard]] bool current_time_between_pos_and_neg_edge(
      const Period& period) const;
  [[nodiscard]] bool current_time_between_neg_and_pos_edge(
      const Period& period) const;

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