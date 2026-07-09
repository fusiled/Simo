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

  [[nodiscard]]
  virtual bool connect(Port* other) = 0;

  [[nodiscard]] std::string_view name() const { return name_; }
  void name(const std::string_view name) { name_ = name; }

 protected:
  std::string name_;
};

namespace Ports {

template <typename Payload>
class InPort;

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

  Payload&& receive() {
    SIMO_ASSERT(connecting_port != nullptr);
    SIMO_ASSERT(connecting_port->state != OutPort<Payload>::PORT_STATE::EMPTY);
    Payload out_p = std::move(connecting_port->storage);
    connecting_port->clear();
    return std::move(out_p);
  }

  Payload* peek() {
    if (connecting_port == nullptr ||
        connecting_port->state() == OutPort<Payload>::PORT_STATE::EMPTY) {
      return nullptr;
    }
    return &connecting_port->storage;
  }

  void clear() { connecting_port->clear(); }

 protected:
  OutPort<Payload>* connecting_port = nullptr;
};

template <typename Payload>
bool OutPort<Payload>::connect(Port* other) {
  if (other->get_type_id() != get_type_id<Port>()) {
    return false;
  }
  auto* other_casted = boost::typeindex::runtime_cast<InPort<Payload>*>(other);
  if (other_casted == nullptr) {
    return false;
  }
  other_casted->connecting_port = this;
  return true;
}

template <typename Payload>
bool InPort<Payload>::connect(Port* other) {
  if (other->get_type_id() != get_type_id<Port>()) {
    return false;
  }
  auto* other_casted = boost::typeindex::runtime_cast<OutPort<Payload>*>(other);
  if (other_casted == nullptr) {
    return false;
  }
  connecting_port = other_casted;
  return true;
}

/// Port that can send and receive payloads on separate channels.
/// It can be connected to a BidirectionalPortTyped<InPayload,OutPayload> (note
/// the types are inverted).
template <typename OutPayload, typename InPayload>
class SIMO_PUBLIC BidirectionalPortTyped : public Port {
 public:
  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(Port)
  bool connect(Port* other) override;

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