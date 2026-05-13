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

#ifndef SIMO_CONFIG_HH
#define SIMO_CONFIG_HH
#include <Simo/Simo.h>

#include <glaze/json/generic.hpp>
#include <string>
#include <vector>

namespace SimoSim::Config {

struct Parameter {
  std::string name;
  glz::generic_u64 value;
};

struct Module {
  std::string name;
  std::string type;
  std::vector<Parameter> parameters;
};

}  // namespace SimoSim::Config

template <>
struct glz::meta<SimoSim::Config::Parameter> {
  using T = SimoSim::Config::Parameter;
  static constexpr auto value = glz::object(&T::name, &T::value);
};

namespace SimoSim::Config {

struct SimulationInfo {
  Simo::Time time;
};

struct Config {
  std::vector<Module> modules;
  std::vector<std::pair<std::string, std::string>> connections;
  SimulationInfo simulation;
};

}  // namespace SimoSim::Config

#endif  // SIMO_CONFIG_HH
