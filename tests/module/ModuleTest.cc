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

#define BOOST_TEST_MODULE Module
#include <Simo/core/Context.h>
#include <Simo/module/Module.h>

#include <filesystem>
#include <fstream>
#include <string>

#include "support/BoostInclude.h"

namespace fs = std::filesystem;

namespace Simo::Tests {
namespace {

[[nodiscard]] std::string read_file_contents(
    const std::filesystem::path& path) {
  std::ifstream input(path);
  std::string contents;
  std::getline(input, contents, '\0');
  return contents;
}

}  // namespace

struct Fixture {
  Fixture() {
    p.name("test_module");
    const auto status = m.initialize(ctx, p);
    BOOST_CHECK(status.success());

    const fs::path temp_dir = fs::temp_directory_path();
    temp_file = temp_dir / "Simo.log";
    std::filesystem::remove(temp_file);
    m.log_setup(temp_file);
    logger = &m.get_logger();
  }

  void flush_log() const { Simo::Log::Logger::flush_sink(temp_file); }

  Simo::Context ctx;
  Simo::Module m;
  Simo::Parameters p;
  fs::path temp_file;
  Simo::Log::Logger* logger;
};

BOOST_FIXTURE_TEST_CASE(EnableLog, Fixture) {
  logger->populate_default_log_levels();
  std::string message = "This is a log message";
  const std::string out_message =
      std::format("[WARNING] [0 ps] [test_module] {}\n", message);
  m.log(3, [&message]() { return message; });
  flush_log();
  const std::string log_content = read_file_contents(temp_file);
  BOOST_CHECK_EQUAL(log_content, out_message);
}

BOOST_FIXTURE_TEST_CASE(LogDisabled, Fixture) {
  logger->populate_default_log_levels();
  m.log_enable(false);
  std::string message = "This is a log message";
  m.log(3, [&message]() { return message; });
  flush_log();
  const std::string log_content = read_file_contents(temp_file);
  BOOST_CHECK_EQUAL(log_content, "");
}

BOOST_FIXTURE_TEST_CASE(LogRaw, Fixture) {
  logger->populate_default_log_levels();
  std::string message = "This is a log message";
  const std::string out_message = std::format("[WARNING] {}", message);
  m.log_raw_callable(3, true, [&message]() { return message; });
  flush_log();
  const std::string log_content = read_file_contents(temp_file);
  BOOST_CHECK_EQUAL(log_content, out_message);
}

BOOST_FIXTURE_TEST_CASE(SkipLogDueToLevel, Fixture) {
  logger->populate_default_log_levels();
  std::string message = "This is a log message";
  logger->log_level(2);
  m.log(1, [&message]() { return message; });
  flush_log();
  const std::string log_content = read_file_contents(temp_file);
  BOOST_CHECK_EQUAL(log_content, "");
}

BOOST_AUTO_TEST_CASE(StdoutLogSetupEnablesLogger) {
  Simo::Log::Logger logger;
  const auto status = logger.initialize("/dev/stdout");

  BOOST_CHECK(status.success());
  BOOST_CHECK_EQUAL(logger.enabled(), true);
}

BOOST_AUTO_TEST_CASE(ModuleChild) {
  Simo::Context ctx;
  Simo::Parameters p;
  Simo::Module m;
  p.name("root");
  const auto status = m.initialize(ctx, p);
  auto& child = m.create_child<Simo::Module>();
  auto child_name = m.name_of_child("child");
  Simo::Parameters p_child;
  p_child.name(child_name);
  auto child_status = child.initialize(ctx, p_child);
  BOOST_CHECK_EQUAL(child_status.success(), true);
  BOOST_CHECK_EQUAL(p_child.name(), "root/child");
}

BOOST_AUTO_TEST_CASE(ModuleGetsPortAsRequestedType) {
  Simo::Module module;
  auto& port =
      module.create_port<Ports::CallbackOutPort<int, bool>>("callback");

  auto* explicitly_typed_port =
      module.get_port<Ports::CallbackOutPort<int, bool>*>("callback");
  auto* inferred_pointer_port =
      module.get_port<Ports::CallbackOutPort<int, bool>>("callback");

  BOOST_CHECK_EQUAL(explicitly_typed_port, &port);
  BOOST_CHECK_EQUAL(inferred_pointer_port, &port);
}

BOOST_AUTO_TEST_CASE(ModuleTypedGetPortReturnsNullForMissingOrWrongType) {
  Simo::Module module;
  module.create_port<Ports::CallbackOutPort<int, bool>>("callback");

  auto* missing_port =
      module.get_port<Ports::CallbackOutPort<int, bool>>("missing");
  auto* wrong_type_port =
      module.get_port<Ports::CallbackInPort<int, bool>>("callback");

  BOOST_CHECK_EQUAL(missing_port, nullptr);
  BOOST_CHECK_EQUAL(wrong_type_port, nullptr);
}

BOOST_AUTO_TEST_CASE(ModuleGetsUnconnectedPorts) {
  Simo::Context context;
  Simo::Module module;
  Simo::Parameters parameters;
  parameters.name("root");
  BOOST_REQUIRE(module.initialize(context, parameters).success());

  auto& connected_out =
      module.create_port<Ports::OutPort<int>>("connected_out");
  auto& connected_in = module.create_port<Ports::InPort<int>>("connected_in");
  BOOST_REQUIRE(connected_out.connect(&connected_in));
  auto& root_unconnected =
      module.create_port<Ports::CallbackOutPort<int, bool>>("unconnected");

  auto& child = module.create_child<Simo::Module>();
  Simo::Parameters child_parameters;
  child_parameters.name(module.name_of_child("child"));
  BOOST_REQUIRE(child.initialize(context, child_parameters).success());
  auto& child_unconnected =
      child.create_port<Ports::CallbackInPort<int, bool>>("unconnected");

  const auto root_ports = module.get_unconnected_ports(false);
  BOOST_REQUIRE_EQUAL(root_ports.size(), 1);
  BOOST_CHECK_EQUAL(root_ports.front().full_name, "root/unconnected");
  BOOST_CHECK_EQUAL(root_ports.front().port, &root_unconnected);

  const auto all_ports = module.get_unconnected_ports(true);
  BOOST_REQUIRE_EQUAL(all_ports.size(), 2);
  BOOST_CHECK_EQUAL(all_ports[0].full_name, "root/unconnected");
  BOOST_CHECK_EQUAL(all_ports[0].port, &root_unconnected);
  BOOST_CHECK_EQUAL(all_ports[1].full_name, "root/child/unconnected");
  BOOST_CHECK_EQUAL(all_ports[1].port, &child_unconnected);
}
}  // namespace Simo::Tests
