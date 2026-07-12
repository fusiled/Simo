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

#define BOOST_TEST_MODULE Port
#include <Simo/port/Port.h>

#include <memory>
#include <string>

#include "support/BoostInclude.h"

namespace Simo::Tests {
namespace {

class NamedPort final : public Simo::Port {
 public:
  [[nodiscard]] bool connect(Simo::Port* other) override {
    return other != nullptr;
  }
};

}  // namespace

BOOST_AUTO_TEST_CASE(PortNameGetterAndSetter) {
  NamedPort left;
  NamedPort right;

  left.name("left_port");
  right.name("right_port");

  BOOST_CHECK_EQUAL(left.name(), "left_port");
  BOOST_CHECK_EQUAL(right.name(), "right_port");
  BOOST_CHECK_EQUAL(left.connect(&right), true);
  BOOST_CHECK_EQUAL(left.connect(nullptr), false);
}

BOOST_AUTO_TEST_CASE(CallbackInPortSendReturnsFalseWhenDisconnected) {
  Ports::CallbackOutPort<int, bool> out;

  BOOST_CHECK(!out.send(1));
}

BOOST_AUTO_TEST_CASE(CallbackInPortSendReturnsFalseWhenOutPortHasNoCallback) {
  Ports::CallbackOutPort<int, bool> out;
  Ports::CallbackInPort<int, bool> in;

  BOOST_CHECK_EQUAL(out.connect(&in), true);
  BOOST_CHECK_EQUAL(in.has_callback(), false);
  BOOST_CHECK(!out.send(1));
}

BOOST_AUTO_TEST_CASE(CallbackInPortSendsPayloadToOutPortCallback) {
  Ports::CallbackOutPort<int, bool> out;
  Ports::CallbackInPort<int, bool> in;
  int received_sum = 0;
  int callback_count = 0;

  in.callback([&](const int payload) {
    received_sum += payload;
    ++callback_count;
    return true;
  });

  BOOST_CHECK_EQUAL(in.has_callback(), true);
  BOOST_CHECK_EQUAL(out.connect(&in), true);
  const auto first_result = out.send(3);
  BOOST_REQUIRE(first_result);
  BOOST_CHECK_EQUAL(*first_result, true);

  constexpr int next_payload = 4;
  const auto result = out.send(next_payload);
  BOOST_REQUIRE(result);
  BOOST_CHECK_EQUAL(*result, true);

  BOOST_CHECK_EQUAL(received_sum, 7);
  BOOST_CHECK_EQUAL(callback_count, 2);
}

BOOST_AUTO_TEST_CASE(CallbackOutPortCanConnectToCallbackInPort) {
  Ports::CallbackOutPort<int, bool> out;
  Ports::CallbackInPort<int, bool> in(
      [](const int /*payload*/) { return true; });

  BOOST_CHECK_EQUAL(in.connect(&out), true);
  auto result = out.send(9);
  BOOST_REQUIRE(result);
  BOOST_CHECK_EQUAL(*result, true);
}

BOOST_AUTO_TEST_CASE(CallbackPortsRejectMismatchedPayloadTypes) {
  Ports::CallbackOutPort<int, bool> int_out;
  Ports::CallbackInPort<int, bool> int_in(
      [](const int /*payload*/) { return true; });
  Ports::CallbackOutPort<std::string, bool> string_in;
  Ports::CallbackInPort<std::string, bool> string_out(
      [](const std::string /*payload*/) { return true; });

  BOOST_CHECK_EQUAL(int_out.connect(&string_out), false);
  BOOST_CHECK_EQUAL(string_out.connect(&int_out), false);
  BOOST_CHECK_EQUAL(string_in.connect(&int_in), false);
  BOOST_CHECK_EQUAL(int_in.connect(&string_in), false);
}

BOOST_AUTO_TEST_CASE(CallbackPortsRejectNullConnections) {
  Ports::CallbackOutPort<int, bool> out;
  Ports::CallbackInPort<int, bool> in(
      [](const int /*payload*/) { return true; });

  BOOST_CHECK_EQUAL(out.connect(nullptr), false);
  BOOST_CHECK_EQUAL(in.connect(nullptr), false);
}

BOOST_AUTO_TEST_CASE(CallbackPortsConnectionDifferentPayloadType) {
  Ports::CallbackOutPort<int, bool> out;
  Ports::CallbackInPort<char, bool> in(
      [](const int /*payload*/) { return true; });
  BOOST_CHECK_EQUAL(out.connect(&in), false);
}

BOOST_AUTO_TEST_CASE(CallbackPortsConnectionDifferentReturnType) {
  Ports::CallbackOutPort<int, bool> out;
  Ports::CallbackInPort<char, void> in([](const int /*payload*/) {});
  BOOST_CHECK_EQUAL(out.connect(&in), false);
}

BOOST_AUTO_TEST_CASE(CallbackPortsConnectionReferencePayload) {
  Ports::CallbackOutPort<int&, void> out;
  Ports::CallbackInPort<int&, void> in([](int& ref) { ref = 1; });
  BOOST_CHECK_EQUAL(out.connect(&in), true);
  int value = 0;
  out.send(value);
  BOOST_CHECK_EQUAL(value, 1);
}

BOOST_AUTO_TEST_CASE(CallbackInPortMovesRvaluePayloadToCallback) {
  Ports::CallbackOutPort<std::unique_ptr<int>, bool> out;
  Ports::CallbackInPort<std::unique_ptr<int>, bool> in;
  int received_value = 0;

  in.callback([&](std::unique_ptr<int> payload) {
    BOOST_REQUIRE_NE(payload, nullptr);
    received_value = *payload;
    return true;
  });

  auto payload = std::make_unique<int>(42);

  BOOST_CHECK_EQUAL(out.connect(&in), true);
  auto result = out.send(std::move(payload));
  BOOST_REQUIRE(result);
  BOOST_CHECK_EQUAL(*result, true);
  BOOST_CHECK_EQUAL(payload, nullptr);
  BOOST_CHECK_EQUAL(received_value, 42);
}

}  // namespace Simo::Tests
