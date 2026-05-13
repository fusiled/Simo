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
#include <Simo/module/core/CoreModules.h>

#include <filesystem>
#include <glaze/yaml.hpp>
#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#define CLI11_HAS_CODECVT 0
#include "CLI11.hpp"
#include "Config.h"

enum RETURN_CODE : int {
  SUCCESS,
  INVALID_COMMAND_LINE,
  INVALID_CONFIG_FILE,
  DUPLICATE_TYPES,
  UNKNOWN_TYPES,
  DUPLICATE_MODULE_NAMES,
  INITIALIZATION_FAILED,
  INVALID_CONNECTION,
  CONNECTION_FAILED
};

using ModuleName = std::string;
using ModuleType = std::string;

struct ModuleParameterPair {
  std::unique_ptr<Simo::Module> module;
  std::unique_ptr<Simo::Parameters> parameters;
};

struct ModulePortName {
  std::string module;
  std::string port;
};

std::optional<ModulePortName> parse_module_port_name(
    const std::string& connection_endpoint) {
  const auto separator = connection_endpoint.find('/');
  if (separator == std::string::npos || separator == 0 ||
      separator == connection_endpoint.size() - 1 ||
      connection_endpoint.find('/', separator + 1) != std::string::npos) {
    return std::nullopt;
  }

  return ModulePortName{
      .module = connection_endpoint.substr(0, separator),
      .port = connection_endpoint.substr(separator + 1),
  };
}

int main(const int argc, char* argv[]) {
  std::filesystem::path config_path;
  std::vector<std::string> collection_search_paths;

  CLI::App app{"Simulation with Simo"};

  // Define options
  app.add_option("--config", config_path, "path to the configuration YAML file")
      ->required()
      ->check(CLI::ExistingFile);
  app.add_option(
         "--search-path", collection_search_paths,
         "directory where to look for shared object containing collections")
      ->check(CLI::ExistingDirectory);

  CLI11_PARSE(app, argc, argv);

  std::vector<Simo::Collections::CollectionWithLib> collections;
  collections.emplace_back(Simo::Modules::Core::loadCoreModules());
  // Map the types to the associated collection
  std::unordered_map<ModuleType, const Simo::Collections::Factory*> factory_map;
  for (const auto& string_path : collection_search_paths) {
    auto error_or_collections =
        Simo::Collections::simo_get_collection_from_folder(string_path);
    if (!error_or_collections) {
      continue;
    }
    collections.append_range(*error_or_collections | std::views::as_rvalue);
  }

  std::vector<ModuleType> duplicate_types;
  for (const auto& c : collections) {
    for (const auto& [type, factory] :
         c.get_collection()->get_factory_pairs()) {
      const ModuleType module_type(type);
      if (factory_map.contains(module_type)) {
        duplicate_types.emplace_back(module_type);
        continue;
      }
      factory_map[module_type] = factory;
    }
  }

  if (!duplicate_types.empty()) {
    std::cerr << "Detected multiple modules with the same type name:\n";
    for (const auto& type : duplicate_types) {
      std::cerr << type << " ";
    }
    std::cerr << "\n";
    return DUPLICATE_TYPES;
  }

  SimoSim::Config::Config cfg;
  if (auto ec = glz::read_file_yaml(cfg, config_path.c_str())) {
    std::cerr << "Error during YAML config parsing of file " << config_path
              << " :\n";
    std::cerr << glz::format_error(ec) << "\n";
    return INVALID_CONFIG_FILE;
  }

  std::vector<std::pair<ModuleName, ModuleType>> unrecognized_module_types;
  std::vector<ModuleName> duplicate_module_names;
  std::unordered_map<ModuleName, ModuleParameterPair> module_map;
  for (const auto& module_spec : cfg.modules) {
    const auto& type = module_spec.type;
    const auto& name = module_spec.name;
    if (!factory_map.contains(type)) {
      unrecognized_module_types.emplace_back(name, type);
      continue;
    }
    if (module_map.contains(name)) {
      duplicate_module_names.emplace_back(name);
      continue;
    }
    const auto& factory = factory_map[type];
    module_map[name] = {.module = factory->get_module(),
                        .parameters = factory->get_parameters()};
    module_map[name].parameters->name(name);

    for (const auto& parameter : module_spec.parameters) {
      auto* p = module_map[name].parameters->get(parameter.name);
      if (p == nullptr) {
        // TODO decide what to do
        std::cerr << "Parameter " << parameter.name << " not found. Ignoring\n";
        continue;
      }
      if (auto v = p->value_from_generic(parameter.value); !v.has_value()) {
        std::cerr << "Parameter " << parameter.name
                  << " cannot be set: " << v.error() << "\n";
      }
    }
  }
  if (!unrecognized_module_types.empty()) {
    std::cerr << "Detected modules with unknown type:\n";
    for (const auto& [name, type] : unrecognized_module_types) {
      std::cerr << name << " " << type << "\n";
    }
    return UNKNOWN_TYPES;
  }

  if (!duplicate_module_names.empty()) {
    std::cerr << "Detected modules with non-unique name:\n";
    for (const auto& name : duplicate_module_names) {
      std::cerr << name << "\n";
    }
    return DUPLICATE_MODULE_NAMES;
  }

  Simo::Context ctx;

  for (const auto& module_parameter_pair : module_map | std::views::values) {
    const auto& [module, parameters] = module_parameter_pair;
    ctx.add(*module, *parameters);
  }

  if (!ctx.initialize()) {
    std::cerr << "Failed to initialize the simulation\n";
    return INITIALIZATION_FAILED;
  }

  for (const auto& [left_endpoint, right_endpoint] : cfg.connections) {
    const auto left = parse_module_port_name(left_endpoint);
    const auto right = parse_module_port_name(right_endpoint);
    if (!left.has_value() || !right.has_value()) {
      std::cerr << "Invalid connection endpoint: " << left_endpoint << " -> "
                << right_endpoint << "\n";
      return INVALID_CONNECTION;
    }

    auto left_module_it = module_map.find(left->module);
    auto right_module_it = module_map.find(right->module);
    if (left_module_it == module_map.end() ||
        right_module_it == module_map.end()) {
      std::cerr << "Connection references unknown module: " << left_endpoint
                << " -> " << right_endpoint << "\n";
      return INVALID_CONNECTION;
    }

    auto* left_port = left_module_it->second.module->get_port(left->port);
    auto* right_port = right_module_it->second.module->get_port(right->port);
    if (left_port == nullptr || right_port == nullptr) {
      std::cerr << "Connection references unknown port: " << left_endpoint
                << " -> " << right_endpoint << "\n";
      return INVALID_CONNECTION;
    }

    if (!left_port->connect(right_port)) {
      std::cerr << "Could not connect ports: " << left_endpoint << " -> "
                << right_endpoint << "\n";
      return CONNECTION_FAILED;
    }
  }

  ctx.run_at(cfg.simulation.time);

  return SUCCESS;
}
