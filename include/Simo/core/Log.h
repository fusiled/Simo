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

#ifndef SIMO_LOG_HH
#define SIMO_LOG_HH
#include <Simo/compiler/Compiler.h>

#include <filesystem>
#include <format>
#include <fstream>
#include <vector>

#include "InitializationStatus.h"

namespace Simo::Log {

enum SIMO_PUBLIC LogLevel : std::uint8_t {
  VERBOSE = 0,
  DEBUG,
  INFO,
  WARNING,
};

class SIMO_PUBLIC Logger {
 public:
  /// Initialize the logger
  InitializationStatus initialize(const std::filesystem::path& sink_path_v);

  /// Add a log level and give it a name
  Logger& add_log_level(size_t level, std::string_view name);

  /// Setup default log levels and set default log level to the highest
  Logger& populate_default_log_levels();

  /// Setup log level explicitly
  Logger& log_level(LogLevel level);
  Logger& log_level(uint8_t level);

  /// Setup log level from string. Do nothing is string is not recognized
  Logger& log_level(std::string_view level_name);

  size_t log_level() const;

  /// Enable/disable logger
  Logger& enabled(bool new_enabled_value);

  bool enabled() const;

  template <typename Callable>
  Logger& log_callable(const size_t level, const bool print_level,
                       Callable&& c) {
    if (!enabled() || level < log_level()) {
      return *this;
    }
    if (print_level) {
      *sink << '[' << level_map[level] << "] ";
    }
    *sink << std::format("{}", c());
    return *this;
  }

  template <typename... Args>
  Logger& log_format(const size_t level, const bool print_level, Args... args) {
    if (!enabled() || level < log_level()) {
      return *this;
    }
    if (print_level) {
      *sink << '[' << level_map[level] << "] ";
    }
    *sink << std::format(std::forward<Args>(args)...);
    return *this;
  }

  ~Logger();

  /// Flush sink at path if this path is identified as a used sink
  static void flush_sink(const std::filesystem::path& path);

  // Flush all the registered sinks
  static void flush_all_sinks();

 protected:
  std::filesystem::path sink_path;
  std::ofstream* sink = nullptr;
  bool is_enabled = false;
  size_t current_log_level = 0;
  std::vector<std::string> level_map;
};

}  // namespace Simo::Log

#endif  // SIMO_LOG_HH