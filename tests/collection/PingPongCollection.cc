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

#include <Simo/Simo.h>

#include <array>
#include <boost/test/detail/log_level.hpp>
#include <cstdint>
#include <print>

#include "Simo/statistics/Count.h"

namespace Simo {
namespace Statistics {
class Count;
}

class Module;
class Parameters;
}  // namespace Simo

extern "C" {

enum struct PingPongMessage : std::uint32_t { PING = 0, PONG = 1 };

class PingPongParameters : public Simo::Parameters {
 public:
  PingPongParameters() {
    // Declare parameters in constructor
    trie.add_unset<Simo::Time>("period").validator(
        [](const auto& t) { return t > Simo::Time::one; });
    trie.add<std::string>("log_level", "WARNING");
    trie.add<std::filesystem::path>("log_file", "Simo.log");
  }
};

class PingModule : public Simo::Module {
 public:
  Simo::InitializationStatus initialize(
      Simo::Context& sim_ctx_p, const Simo::Parameters& parameters) override {
    if (const auto status = Module::initialize(sim_ctx_p, parameters);
        !status.success()) {
      return status;
    }
    log_setup(parameters.get<std::filesystem::path>("log_file")->value());
    log_level(parameters.get<std::string>("log_level")->value());
    period = parameters.get<Simo::Time>("period")->value();
    port =
        &create_port<Simo::Ports::BidirectionalPort<PingPongMessage>>("port");
    num_msg_sent = &create_statistic<Simo::Statistics::Count>("num_msg_sent");

    sim_ctx().schedule_at(Simo::Time::zero, [this]() { update_state(); });
    sim_ctx().schedule_at(period / 2, [this]() { try_send(); });
    return Simo::InitializationStatus::ok(this);
  }

  void update_state() {
    sim_ctx().schedule_in(period, [this]() { update_state(); });
    send_message = start_send;
    if (const auto* const val = port->peek_in();
        val != nullptr && *val == PingPongMessage::PONG) {
      port->clear_in();
      send_message = true;
    }
  }

  void try_send() {
    sim_ctx().schedule_in(period, [this]() { try_send(); });
    if (send_message) {
      log(Simo::Log::LogLevel::INFO, []() { return "Send PING"; });
      port->send_out(PingPongMessage::PING);
      ++(*num_msg_sent);
      start_send = false;
    }
  }

 protected:
  Simo::Ports::BidirectionalPort<PingPongMessage>* port = nullptr;
  Simo::Statistics::Count* num_msg_sent = nullptr;
  bool send_message = false;
  bool start_send = true;
  Simo::Time period = Simo::Time::zero;
};

class PongModule : public Simo::Module {
 public:
  Simo::InitializationStatus initialize(
      Simo::Context& sim_ctx_v, const Simo::Parameters& parameters) override {
    if (const auto status = Module::initialize(sim_ctx_v, parameters);
        !status.success()) {
      return status;
    }
    period = parameters.get<Simo::Time>("period")->value();
    log_setup(parameters.get<std::filesystem::path>("log_file")->value());
    log_level(parameters.get<std::string>("log_level")->value());
    port =
        &create_port<Simo::Ports::BidirectionalPort<PingPongMessage>>("port");
    num_msg_sent = &create_statistic<Simo::Statistics::Count>("num_msg_sent");
    sim_ctx().schedule_at(Simo::Time::zero, [this]() { update_state(); });
    sim_ctx().schedule_at(period / 2, [this]() { try_send(); });

    return Simo::InitializationStatus::ok(this);
  }

  void update_state() {
    sim_ctx().schedule_in(period, [this]() { update_state(); });
    send_message = false;
    if (const auto val = port->peek_in();
        val != nullptr && *val == PingPongMessage::PING) {
      port->clear_in();
      send_message = true;
    }
  }

  void try_send() {
    sim_ctx().schedule_in(period, [this]() { try_send(); });
    if (send_message) {
      log(Simo::Log::LogLevel::INFO, []() { return "Send PONG"; });
      port->send_out(PingPongMessage::PONG);
      ++(*num_msg_sent);
    }
  }

 protected:
  Simo::Ports::BidirectionalPort<PingPongMessage>* port = nullptr;
  Simo::Statistics::Count* num_msg_sent = nullptr;
  Simo::Time period = Simo::Time::zero;
  bool send_message = false;
};

constexpr Simo::Collections::SimoCollectionVersion VERSION{
    .major = 0, .minor = 0, .patch = 1};

constexpr Simo::Collections::Factory PING_FACTORY{
    .name = "ping",
    .get_module_function =
        [] { return static_cast<Simo::Module*>(new PingModule()); },
    .get_parameters_function =
        [] {
          return static_cast<Simo::Parameters*>(new PingPongParameters());
        }};

constexpr Simo::Collections::Factory PONG_FACTORY{
    .name = "pong",
    .get_module_function =
        [] { return static_cast<Simo::Module*>(new PongModule()); },
    .get_parameters_function =
        [] {
          return static_cast<Simo::Parameters*>(new PingPongParameters());
        }};

const char* collection_name = "PingPongCollection";
constexpr std::array FACTORY_LIST{PING_FACTORY, PONG_FACTORY};
static Simo::Collections::SimoCollection collection{
    .name = collection_name,
    .version = VERSION,
    .factory_list = FACTORY_LIST.data(),
    .factory_list_size = FACTORY_LIST.size(),
};

const Simo::Collections::SimoCollection* simo_get_collection() {
  return &collection;
}
}