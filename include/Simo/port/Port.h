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

#ifndef SIMO_PORT_HH
#define SIMO_PORT_HH

#include <expected>
#include <functional>
#include <utility>

#include "Simo/compiler/BoostTypeIndexRuntimeCast.h"
#include "Simo/compiler/Compiler.h"

namespace Simo {

/// Generic port. It offers the virtual method connect
class SIMO_PUBLIC Port {
 public:
  virtual ~Port() = default;
  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS()

  using TypeId = boost::typeindex::type_index;

  /// Utility to the get type of class. Need the type at compile time
  template <typename Self>
  TypeId get_type_id(this Self& /*unused*/) {
    return boost::typeindex::type_id<Self>();
  }

  TypeId get_runtime_type() const {
    return boost::typeindex::type_id_runtime(*this);
  }

  [[nodiscard]]
  virtual bool connect(Port* other) = 0;

  virtual bool connected() const = 0;

  [[nodiscard]] std::string_view name() const { return name_; }
  void name(const std::string_view name) { name_ = name; }

 protected:
  std::string name_;
};

namespace Ports {

template <typename Payload>
class InPort;

template <typename Payload, typename ReturnType>
class CallbackOutPort;

/// Templated port that can send payloads to an InPort of the same type
///
/// Present the payload to the connected port with send and clear that state
/// with the clear method
template <typename Payload>
class SIMO_PUBLIC OutPort : public Port {
 public:
  friend class InPort<Payload>;

  enum struct PORT_STATE : std::uint8_t {
    EMPTY,
    FILLED,
  };

  enum struct SEND_OUTCOME : std::uint8_t {
    NEW,
    REPLACED,
  };

  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(Port)
  [[nodiscard]]
  bool connect(Port* other) override;

  [[nodiscard]]
  bool connected() const override {
    return connecting_port != nullptr;
  }

  SEND_OUTCOME send(Payload&& payload) {
    storage = std::move(payload);
    switch (state_) {
      case PORT_STATE::EMPTY:
        state_ = PORT_STATE::FILLED;
        return SEND_OUTCOME::NEW;
      case PORT_STATE::FILLED:
        state_ = PORT_STATE::FILLED;
        return SEND_OUTCOME::REPLACED;
    }
    std::abort();
  }

  SEND_OUTCOME send(Payload& payload) {
    storage = payload;
    switch (state_) {
      case PORT_STATE::EMPTY:
        state_ = PORT_STATE::FILLED;
        return SEND_OUTCOME::NEW;
      case PORT_STATE::FILLED:
        state_ = PORT_STATE::FILLED;
        return SEND_OUTCOME::REPLACED;
    }
    std::abort();
  }

  void clear() { state_ = PORT_STATE::EMPTY; }

  PORT_STATE state() const { return state_; }

 protected:
  Payload storage;
  PORT_STATE state_ = PORT_STATE::EMPTY;
  InPort<Payload>* connecting_port = nullptr;
};

/// Templated port that can received payloads from an OutPort of the same type
///
/// A payload can be extracted with the receive method. The peek method allows
/// to look at the payload without extracting it.
template <typename Payload>
class SIMO_PUBLIC InPort : public Port {
 public:
  friend class OutPort<Payload>;
  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(Port)
  [[nodiscard]]
  bool connect(Port* other) override;

  [[nodiscard]]
  bool connected() const override {
    return connecting_port != nullptr;
  }

  Payload receive() {
    SIMO_ASSERT(connecting_port != nullptr);
    SIMO_ASSERT(connecting_port->state() !=
                OutPort<Payload>::PORT_STATE::EMPTY);
    Payload out_p = std::move(connecting_port->storage);
    connecting_port->clear();
    return out_p;
  }

  Payload* peek() {
    if (connecting_port == nullptr ||
        connecting_port->state() == OutPort<Payload>::PORT_STATE::EMPTY) {
      return nullptr;
    }
    return &connecting_port->storage;
  }

  void clear() {
    if (connecting_port != nullptr) {
      connecting_port->clear();
    }
  }

 protected:
  OutPort<Payload>* connecting_port = nullptr;
};

/// Templated output port that receives payloads from a CallbackInPort of the
/// same type and invokes a callback for each payload.
template <typename Payload, typename ReturnType>
class SIMO_PUBLIC CallbackInPort : public Port {
 public:
  friend class CallbackOutPort<Payload, ReturnType>;

  using Callback = std::function<ReturnType(Payload)>;

  CallbackInPort() = default;
  explicit CallbackInPort(Callback callback) : callback_(std::move(callback)) {}

  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(Port)
  [[nodiscard]]
  bool connect(Port* other) override;

  [[nodiscard]]
  bool connected() const override {
    return connecting_port != nullptr;
  }

  void callback(Callback callback) { callback_ = std::move(callback); }

  [[nodiscard]]
  bool has_callback() const {
    return static_cast<bool>(callback_);
  }

 protected:
  void receive(Payload payload)
    requires(std::is_same_v<ReturnType, void>)
  {
    if (callback_) {
      callback_(std::forward<Payload>(payload));
    }
  }

  [[nodiscard]]
  std::optional<ReturnType> receive(Payload payload)
    requires(!std::is_same_v<ReturnType, void>)
  {
    if (callback_) {
      return callback_(std::forward<Payload>(payload));
    }
    return std::nullopt;
  }

  Callback callback_;
  CallbackOutPort<Payload, ReturnType>* connecting_port = nullptr;
};

enum struct SIMO_PUBLIC VERIFY_CONTRACT_ERROR : std::uint8_t {
  NO_CONNECTED_PORT,
  NO_CONNECTED_PORT_CONTRACT,
  NO_PREDICATE,
};

/// Class to implement the contract logic.
///
/// A contract is just a structure that a connecting port can probe
/// And validate using a predicate
template <typename Contract>
class SIMO_PUBLIC ContractInterface {
 public:
  std::optional<Contract> contract() { return contract_; }
  void contract(Contract contract) { contract_ = contract; }

  void connected_port(ContractInterface* port) { connected_port_ = port; }

  void connected_port_contract_predicate(
      std::function<bool(Contract)> predicate) {
    connected_port_contract_predicate_ = predicate;
  }
  std::expected<bool, VERIFY_CONTRACT_ERROR> verify_connected_port_contract() {
    if (connected_port_ == nullptr) {
      return std::unexpected(VERIFY_CONTRACT_ERROR::NO_CONNECTED_PORT);
    }
    if (connected_port_->contract() == std::nullopt) {
      return std::unexpected(VERIFY_CONTRACT_ERROR::NO_CONNECTED_PORT_CONTRACT);
    }
    if (!connected_port_contract_predicate_) {
      return std::unexpected(VERIFY_CONTRACT_ERROR::NO_PREDICATE);
    }
    return connected_port_contract_predicate_(*connected_port_->contract());
  }

 protected:
  std::optional<Contract> contract_;
  ContractInterface* connected_port_ = nullptr;
  std::function<bool(Contract)> connected_port_contract_predicate_{};
};

/// Callback out port with contract logic.
///
/// See CallbackOutPort and ContractInterface classes
template <typename Payload, typename ReturnType, typename Contract>
class SIMO_PUBLIC CallbackContractOutPort
    : public CallbackOutPort<Payload, ReturnType>,
      public ContractInterface<Contract> {
 public:
  CallbackContractOutPort() = default;

  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(CallbackOutPort<Payload, ReturnType>)
  [[nodiscard]]
  bool connect(Port* other) override;
};

/// Callback out port with contract logic.
///
/// See CallbackInPort and ContractInterface classes
template <typename Payload, typename ReturnType, typename Contract>
class SIMO_PUBLIC CallbackContractInPort
    : public CallbackInPort<Payload, ReturnType>,
      public ContractInterface<Contract> {
 public:
  CallbackContractInPort() = default;

  explicit CallbackContractInPort(
      CallbackInPort<Payload, ReturnType>::Callback callback)
      : CallbackInPort<Payload, ReturnType>(std::move(callback)) {}

  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(CallbackInPort<Payload, ReturnType>)
  [[nodiscard]]
  bool connect(Port* other) override;
};

/// Templated input port that sends payloads to a CallbackOutPort of the same
/// type.
template <typename Payload, typename ReturnType>
class SIMO_PUBLIC CallbackOutPort : public Port {
 public:
  friend class CallbackInPort<Payload, ReturnType>;
  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(Port)
  [[nodiscard]]
  bool connect(Port* other) override;

  [[nodiscard]]
  bool connected() const override {
    return connecting_port != nullptr;
  }

  template <typename Arg>
    requires std::constructible_from<Payload, Arg&&> &&
             (!std::is_void_v<ReturnType>)
  [[nodiscard]]
  std::optional<ReturnType> send(Arg&& payload) {
    if (connecting_port == nullptr) {
      return std::nullopt;
    }

    return connecting_port->receive(std::forward<Arg>(payload));
  }

  template <typename Arg>
    requires std::constructible_from<Payload, Arg&&> &&
             std::is_void_v<ReturnType>
  void send(Arg&& payload) {
    if (connecting_port == nullptr || !connecting_port->has_callback()) {
      return;
    }

    connecting_port->receive(std::forward<Arg>(payload));
  }

 protected:
  CallbackInPort<Payload, ReturnType>* connecting_port = nullptr;
};

template <typename Payload>
bool OutPort<Payload>::connect(Port* other) {
  if (other == nullptr) {
    return false;
  }
  auto* other_casted = boost::typeindex::runtime_cast<InPort<Payload>*>(other);
  if (other_casted == nullptr) {
    return false;
  }
  connecting_port = other_casted;
  other_casted->connecting_port = this;
  return true;
}

template <typename Payload>
bool InPort<Payload>::connect(Port* other) {
  if (other == nullptr) {
    return false;
  }
  auto* other_casted = boost::typeindex::runtime_cast<OutPort<Payload>*>(other);
  if (other_casted == nullptr) {
    return false;
  }
  connecting_port = other_casted;
  other_casted->connecting_port = this;
  return true;
}

template <typename Payload, typename ReturnType>
bool CallbackOutPort<Payload, ReturnType>::connect(Port* other) {
  if (other == nullptr) {
    return false;
  }
  auto* other_casted =
      boost::typeindex::runtime_cast<CallbackInPort<Payload, ReturnType>*>(
          other);
  if (other_casted == nullptr) {
    return false;
  }
  connecting_port = other_casted;
  other_casted->connecting_port = this;
  return true;
}

template <typename Payload, typename ReturnType>
bool CallbackInPort<Payload, ReturnType>::connect(Port* other) {
  if (other == nullptr) {
    return false;
  }
  auto* other_casted =
      boost::typeindex::runtime_cast<CallbackOutPort<Payload, ReturnType>*>(
          other);
  if (other_casted == nullptr) {
    return false;
  }
  connecting_port = other_casted;
  other_casted->connecting_port = this;
  return true;
}

template <typename Payload, typename ReturnType, typename Contract>
bool CallbackContractOutPort<Payload, ReturnType, Contract>::connect(
    Port* other) {
  if (other == nullptr) {
    return false;
  }
  auto* other_casted = boost::typeindex::runtime_cast<
      CallbackContractInPort<Payload, ReturnType, Contract>*>(other);
  if (other_casted == nullptr) {
    return false;
  }
  ContractInterface<Contract>::connected_port(other_casted);
  other_casted->connected_port(this);
  return CallbackOutPort<Payload, ReturnType>::connect(other_casted);
}

template <typename Payload, typename ReturnType, typename Contract>
bool CallbackContractInPort<Payload, ReturnType, Contract>::connect(
    Port* other) {
  if (other == nullptr) {
    return false;
  }
  auto* other_casted = boost::typeindex::runtime_cast<
      CallbackContractOutPort<Payload, ReturnType, Contract>*>(other);
  if (other_casted == nullptr) {
    return false;
  }
  ContractInterface<Contract>::connected_port(other_casted);
  other_casted->connected_port(this);
  return CallbackInPort<Payload, ReturnType>::connect(other_casted);
}

/// Port that can send and receive payloads on separate channels.
/// It can be connected to a BidirectionalPortTyped<InPayload,OutPayload> (note
/// the types are inverted).
template <typename OutPayload, typename InPayload>
class SIMO_PUBLIC BidirectionalPortTyped : public Port {
 public:
  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(Port)
  bool connect(Port* other) override;

  [[nodiscard]]
  bool connected() const override {
    return out_port.connected() && in_port.connected();
  }

  /// Push a payload on the out port
  OutPort<OutPayload>::SEND_OUTCOME send_out(OutPayload&& payload) {
    return out_port.send(std::move(payload));
  }

  OutPort<OutPayload>::SEND_OUTCOME send_out(OutPayload& payload) {
    return out_port.send(payload);
  }

  void clear_out() { out_port.clear(); }

  void clear_in() { in_port.clear(); }

  /// Extract the payload from the in port
  InPayload&& receive_in() { return in_port.receive(); }

  /// Peek the payload from the in port
  InPayload* peek_in() { return in_port.peek(); }

 protected:
  OutPort<OutPayload> out_port;
  InPort<InPayload> in_port;
};

template <typename Payload>
using BidirectionalPort = BidirectionalPortTyped<Payload, Payload>;

template <typename OutPayload, typename InPayload>
bool BidirectionalPortTyped<OutPayload, InPayload>::connect(Port* other) {
  auto* other_casted = boost::typeindex::runtime_cast<
      BidirectionalPortTyped<InPayload, OutPayload>*>(other);
  if (other_casted == nullptr) {
    return false;
  }
  return out_port.connect(&other_casted->in_port) &&
         in_port.connect(&other_casted->out_port);
}

}  // namespace Ports

}  // namespace Simo

#endif  // SIMO_PORT_HH
