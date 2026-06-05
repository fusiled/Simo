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

#include <Simo/core/Log.h>

#include <ranges>
#include <unordered_map>

#include "third-party/enchantum_single_header.hpp"

namespace Simo::Log {

struct SinkTracker {
  std::ofstream sink;
  int ref_count = 0;
};

static std::unordered_map<std::filesystem::path, SinkTracker> sink_tracker;

InitializationStatus Logger::initialize(
    const std::filesystem::path& sink_path_v) {
  sink_path = sink_path_v;
  if (sink_tracker.contains(sink_path)) {
    // Sink file was already opened
    sink_tracker[sink_path].ref_count++;
    sink = &sink_tracker[sink_path].sink;
    is_enabled = true;
    return InitializationStatus::ok(nullptr);
  }
  std::ofstream new_sink;
  new_sink.open(sink_path, std::ios::out | std::ios::trunc);
  if (!new_sink.is_open()) {
    return InitializationStatus(
        nullptr, {"Failed to open log file: " + sink_path.string()});
  }
  sink_tracker[sink_path] = {.sink = std::move(new_sink), .ref_count = 1};
  sink = &sink_tracker[sink_path].sink;
  is_enabled = true;
  return InitializationStatus::ok(nullptr);
}

Logger& Logger::add_log_level(const size_t level, const std::string_view name) {
  level_map.resize(std::max(level_map.size(), level + 1), "");
  level_map[level] = name;
  return *this;
}

Logger& Logger::populate_default_log_levels() {
  level_map.clear();
  level_map.resize(enchantum::count<LogLevel>);
  for (const auto [idx, name] : enchantum::entries_generator<LogLevel>) {
    level_map[idx] = name;
  }
  log_level(
      static_cast<LogLevel>(level_map.size() - 1));  // Print only warnings
  return *this;
}

Logger& Logger::log_level(const LogLevel level) {
  current_log_level = level;
  return *this;
}

Logger& Logger::log_level(const uint8_t level) {
  current_log_level = level;
  return *this;
}

Logger& Logger::log_level(const std::string_view level_name) {
  for (size_t i = 0; i < level_map.size(); i++) {
    if (level_map[i] == level_name) {
      current_log_level = i;
      break;
    }
  }
  return *this;
}

size_t Logger::log_level() const { return current_log_level; }

Logger& Logger::enabled(const bool new_enabled_value) {
  is_enabled = new_enabled_value;
  return *this;
}

bool Logger::enabled() const { return is_enabled; }

Logger::~Logger() {
  if (!sink_tracker.contains(sink_path)) {
    return;
  }
  int& ref_count = sink_tracker[sink_path].ref_count;
  ref_count--;
  if (ref_count <= 0) {
    sink_tracker[sink_path].sink.flush();
    sink_tracker[sink_path].sink.close();
    sink_tracker.erase(sink_path);
  }
}

void Logger::flush_sink(const std::filesystem::path& path) {
  if (!sink_tracker.contains(path)) {
    return;
  }
  sink_tracker[path].sink << std::flush;
}

void Logger::flush_all_sinks() {
  for (auto& tracker : sink_tracker | std::views::values) {
    tracker.sink << std::flush;
  }
}

}  // namespace Simo::Log