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

  [[nodiscard]] bool connected() const override { return false; }
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

BOOST_AUTO_TEST_CASE(PortsReportTheirConnectionState) {
  Ports::OutPort<int> out;
  Ports::InPort<int> in;
  BOOST_CHECK(!out.connected());
  BOOST_CHECK(!in.connected());
  BOOST_REQUIRE(out.connect(&in));
  BOOST_CHECK(out.connected());
  BOOST_CHECK(in.connected());

  Ports::CallbackOutPort<int, bool> callback_out;
  Ports::CallbackInPort<int, bool> callback_in;
  BOOST_CHECK(!callback_out.connected());
  BOOST_CHECK(!callback_in.connected());
  BOOST_REQUIRE(callback_in.connect(&callback_out));
  BOOST_CHECK(callback_out.connected());
  BOOST_CHECK(callback_in.connected());

  Ports::CallbackContractOutPort<int, bool, int> contract_out;
  Ports::CallbackContractInPort<int, bool, int> contract_in;
  BOOST_CHECK(!contract_out.connected());
  BOOST_CHECK(!contract_in.connected());
  BOOST_REQUIRE(contract_out.connect(&contract_in));
  BOOST_CHECK(contract_out.connected());
  BOOST_CHECK(contract_in.connected());

  Ports::BidirectionalPort<int> bidirectional_left;
  Ports::BidirectionalPort<int> bidirectional_right;
  BOOST_CHECK(!bidirectional_left.connected());
  BOOST_CHECK(!bidirectional_right.connected());
  BOOST_REQUIRE(bidirectional_left.connect(&bidirectional_right));
  BOOST_CHECK(bidirectional_left.connected());
  BOOST_CHECK(bidirectional_right.connected());
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

BOOST_AUTO_TEST_CASE(CallbackContractPortsConnectionSuccess) {
  Ports::CallbackContractOutPort<int&, void, int> out;
  // No port connected returns true
  BOOST_CHECK(out.verify_connected_port_contract().error() ==
              Ports::VERIFY_CONTRACT_ERROR::NO_CONNECTED_PORT);
  Ports::CallbackContractInPort<int&, void, int> in([](int& ref) { ref = 1; });
  BOOST_CHECK_EQUAL(out.connect(&in), true);
  // No contract set, returns true
  BOOST_CHECK(out.verify_connected_port_contract().error() ==
              Ports::VERIFY_CONTRACT_ERROR::NO_CONNECTED_PORT_CONTRACT);
  in.contract(10);
  // A predicate to verify the contract is not set yet, so return true
  BOOST_CHECK(out.verify_connected_port_contract().error() ==
              Ports::VERIFY_CONTRACT_ERROR::NO_PREDICATE);
  out.connected_port_contract_predicate([](int v) { return v == 9; });
  // Predicate not satisfied
  BOOST_CHECK_EQUAL(out.verify_connected_port_contract().value(), false);
  in.contract(9);
  BOOST_CHECK_EQUAL(out.verify_connected_port_contract().value(), true);
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

BOOST_AUTO_TEST_CASE(GetRuntimeType) {
  Ports::CallbackOutPort<std::unique_ptr<int>, bool> port;
  Port* casted_port = &port;
  BOOST_CHECK_EQUAL(port.get_runtime_type(), casted_port->get_runtime_type());
}

}  // namespace Simo::Tests
