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

#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <fstream>
#include <string>

namespace fs = std::filesystem;

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

BOOST_AUTO_TEST_CASE(NameOfChild) {
  Simo::Context ctx;
  Simo::Parameters p;
  Simo::Module m;
  p.name("root");
  const auto status = m.initialize(ctx, p);
  auto child_name = m.name_of_child("child");
  BOOST_CHECK_EQUAL(child_name, "root/child");
}
