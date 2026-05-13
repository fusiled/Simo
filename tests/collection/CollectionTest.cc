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

#define BOOST_TEST_MODULE SimoCollections
#include <Simo/Simo.h>

#include <boost/test/unit_test.hpp>
#include <filesystem>
#include <memory>
#include <string_view>
#include <utility>

#if defined(__APPLE__)
#define DYNLIB_EXT "dylib"
#else
#define DYNLIB_EXT "so"
#endif

namespace {
std::filesystem::path collection_library_path() {
  // Assuming tests are run from the build folder
  return std::filesystem::path("./libSimoTestPingPongCollection." DYNLIB_EXT);
}
}  // namespace

BOOST_AUTO_TEST_CASE(dynamic_load_test) {
  using namespace Simo::Collections;
  using Simo::Module;
  using Simo::Parameters;
  using Simo::Period;
  using Simo::Time;
  // Assuming to run this inside the build folder
  const std::filesystem::path collection_file =
      "./libSimoTestPingPongCollection." DYNLIB_EXT;
  const auto expected_collection = simo_get_collection(collection_file);
  BOOST_CHECK_EQUAL(expected_collection.has_value(), true);
  const CollectionWithLib& collection_with_lib = expected_collection.value();
  BOOST_CHECK_NE(collection_with_lib.get_collection(), nullptr);
  const SimoCollection* collection = collection_with_lib.get_collection();
  BOOST_CHECK_EQUAL(collection->name, "PingPongCollection");
  BOOST_CHECK(collection->version == SimoCollectionVersion(0, 0, 1));

  const auto* ping_factory_ptr = collection->get_factory("ping");
  BOOST_CHECK_NE(ping_factory_ptr, nullptr);
  const auto& ping_factory = *ping_factory_ptr;
  const auto* pong_factory_ptr = collection->get_factory("pong");
  BOOST_CHECK_NE(pong_factory_ptr, nullptr);
  const auto& pong_factory = *pong_factory_ptr;

  auto ping_module = ping_factory.get_module();
  auto pong_module = pong_factory.get_module();
  // Ping and Pong modules can use the same parameters
  auto params = ping_factory.get_parameters();

  params->name("ping");
  params->set<Time>("period", Time(10));

  Simo::Context sim_ctx;
  sim_ctx.add(*ping_module, *params);
  sim_ctx.add(*pong_module, *params);

  BOOST_CHECK_EQUAL(sim_ctx.initialize(), true);

  auto* ping_port = ping_module->get_port("port");
  BOOST_CHECK_NE(ping_port, nullptr);
  auto* pong_port = pong_module->get_port("port");
  BOOST_CHECK_NE(pong_port, nullptr);

  const bool connection_ok = ping_port->connect(pong_port);
  BOOST_CHECK_EQUAL(connection_ok, true);

  auto* ping_stat =
      ping_module->get_statistic<Simo::Statistics::Count>("num_msg_sent");
  auto* pong_stat =
      pong_module->get_statistic<Simo::Statistics::Count>("num_msg_sent");
  BOOST_CHECK_NE(ping_stat, nullptr);
  BOOST_CHECK_NE(pong_stat, nullptr);

  sim_ctx.run_at(Time::one);
  // Packets are put on the flop at negedge
  // Every cycle, 1 transaction takes place
  BOOST_CHECK_EQUAL(ping_stat->value(), 0);
  BOOST_CHECK_EQUAL(pong_stat->value(), 0);
  sim_ctx.run_at(Time(10));
  BOOST_CHECK_EQUAL(ping_stat->value(), 1);
  BOOST_CHECK_EQUAL(pong_stat->value(), 0);
  sim_ctx.run_at(Time(20));
  BOOST_CHECK_EQUAL(ping_stat->value(), 1);
  BOOST_CHECK_EQUAL(pong_stat->value(), 1);
  sim_ctx.run_at(Time(30));
  BOOST_CHECK_EQUAL(ping_stat->value(), 2);
  BOOST_CHECK_EQUAL(pong_stat->value(), 1);
  sim_ctx.run_at(Time(40));
  BOOST_CHECK_EQUAL(ping_stat->value(), 2);
  BOOST_CHECK_EQUAL(pong_stat->value(), 2);
}

BOOST_AUTO_TEST_CASE(dynamic_load_test_failure) {
  using Simo::Time;
  using Simo::Collections::simo_get_collection;

  auto collection = simo_get_collection("./aklssksk");
  BOOST_CHECK_EQUAL(collection.has_value(), false);
}

BOOST_AUTO_TEST_CASE(dynamic_load_test_folder) {
  using Simo::Collections::simo_get_collection_from_folder;

  // Assuming to run this inside the build folder
  auto result = simo_get_collection_from_folder("./");
  BOOST_CHECK(result.has_value());
  auto& collection_v = result.value();
  // The build folder may be spurious with extra libs, so theere may
  // be multiple collections
  BOOST_CHECK_GE(collection_v.size(), 1);

  for (auto& collection_with_lib : collection_v) {
    const auto* collection = collection_with_lib.get_collection();
    if (std::string_view(collection->name) == "PingPongCollection") {
      return;
    }
  }
  BOOST_FAIL("PingPongCollection not detected");
}

BOOST_AUTO_TEST_CASE(dynamic_load_and_factory_safe_unsafe_apis) {
  using namespace Simo::Collections;
  using Simo::Period;
  using Simo::Time;

  const auto collection_file = collection_library_path();
  auto expected_collection = simo_get_collection(collection_file);
  BOOST_REQUIRE(expected_collection.has_value());

  const CollectionWithLib& collection_with_lib = expected_collection.value();
  const SimoCollection* collection = collection_with_lib.get_collection();
  BOOST_REQUIRE_NE(collection, nullptr);

  BOOST_CHECK_EQUAL(collection->name, "PingPongCollection");
  BOOST_CHECK(collection->version == SimoCollectionVersion(0, 0, 1));
  collection->check();
  BOOST_CHECK_EQUAL(collection->get_factory("missing"), nullptr);

  const auto* ping_factory_ptr = collection->get_factory("ping");
  BOOST_REQUIRE_NE(ping_factory_ptr, nullptr);
  const auto& ping_factory = *ping_factory_ptr;
  BOOST_CHECK_EQUAL(ping_factory.get_name(), "ping");

  // Safe APIs
  auto safe_module = ping_factory.get_module();
  auto safe_params = ping_factory.get_parameters();
  BOOST_REQUIRE_NE(safe_module, nullptr);
  BOOST_REQUIRE_NE(safe_params, nullptr);

  // Unsafe APIs
  std::unique_ptr<Simo::Module> raw_module(ping_factory.get_module_unsafe());
  std::unique_ptr<Simo::Parameters> raw_params(
      ping_factory.get_parameters_unsafe());
  BOOST_REQUIRE_NE(raw_module, nullptr);
  BOOST_REQUIRE_NE(raw_params, nullptr);

  // Basic initialization path with unsafe factory objects
  raw_params->name("unsafe_ping");
  raw_params->set<Time>("period", Time(10));

  Simo::Context sim_ctx;
  sim_ctx.add(*raw_module, *raw_params);
  BOOST_CHECK_EQUAL(sim_ctx.initialize(), true);
  sim_ctx.run_at(Time::one);
}

BOOST_AUTO_TEST_CASE(dynamic_load_error_code_dlopen_fail) {
  using Simo::Collections::GET_COLLECTION_ERROR;
  using Simo::Collections::simo_get_collection;

  auto missing = simo_get_collection("./this_file_does_not_exist_12345.so");
  BOOST_REQUIRE(!missing.has_value());
  BOOST_CHECK(missing.error().error_code == GET_COLLECTION_ERROR::DLOPEN_FAIL);
}

BOOST_AUTO_TEST_CASE(dynamic_load_error_code_already_loaded_library) {
  using Simo::Collections::GET_COLLECTION_ERROR;
  using Simo::Collections::simo_get_collection;

  const auto collection_file = collection_library_path();

  auto first = simo_get_collection(collection_file);
  BOOST_REQUIRE(first.has_value());

  auto second = simo_get_collection(collection_file);
  BOOST_REQUIRE(!second.has_value());
  BOOST_CHECK(second.error().error_code ==
              GET_COLLECTION_ERROR::ALREADY_LOADED_LIBRARY);
}

BOOST_AUTO_TEST_CASE(dynamic_load_from_folder_success_and_not_directory_error) {
  using Simo::Collections::GET_COLLECTION_ERROR;
  using Simo::Collections::simo_get_collection_from_folder;

  auto result = simo_get_collection_from_folder("./");
  BOOST_REQUIRE(result.has_value());
  auto& collection_v = result.value();
  BOOST_CHECK_GE(collection_v.size(), 1U);

  bool found_ping_pong = false;
  for (auto& collection_with_lib : collection_v) {
    const auto* collection = collection_with_lib.get_collection();
    BOOST_REQUIRE_NE(collection, nullptr);
    if (std::string_view(collection->name) == "PingPongCollection") {
      found_ping_pong = true;
    }
  }
  BOOST_CHECK_EQUAL(found_ping_pong, true);

  const auto collection_file = collection_library_path();
  auto not_directory = simo_get_collection_from_folder(collection_file);
  BOOST_REQUIRE(!not_directory.has_value());
  BOOST_CHECK(not_directory.error().error_code ==
              GET_COLLECTION_ERROR::NOT_A_DIRECTORY);
}

BOOST_AUTO_TEST_CASE(
    collection_with_lib_move_constructor_preserves_collection) {
  using Simo::Collections::CollectionWithLib;
  using Simo::Collections::simo_get_collection;

  auto loaded = simo_get_collection(collection_library_path());
  BOOST_REQUIRE(loaded.has_value());

  const auto* original_collection = loaded.value().get_collection();
  BOOST_REQUIRE_NE(original_collection, nullptr);

  CollectionWithLib moved(std::move(loaded.value()));
  BOOST_CHECK_EQUAL(moved.get_collection(), original_collection);
}

BOOST_AUTO_TEST_CASE(collection_with_lib_move_assignment_and_self_move) {
  using Simo::Collections::CollectionWithLib;
  using Simo::Collections::simo_get_collection;

  auto loaded = simo_get_collection(collection_library_path());
  BOOST_REQUIRE(loaded.has_value());

  CollectionWithLib source(std::move(loaded.value()));
  const auto* source_collection = source.get_collection();
  BOOST_REQUIRE_NE(source_collection, nullptr);

  CollectionWithLib destination(std::filesystem::path("dummy"), nullptr,
                                nullptr);
  destination = std::move(source);
  BOOST_CHECK_EQUAL(destination.get_collection(), source_collection);

  BOOST_CHECK_EQUAL(destination.get_collection(), source_collection);
}
