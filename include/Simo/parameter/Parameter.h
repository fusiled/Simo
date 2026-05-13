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

#ifndef SIMO_PARAMETER_HH
#define SIMO_PARAMETER_HH

#include <Simo/compiler/BoostTypeIndexRuntimeCast.h>

#include <algorithm>
#include <functional>
#include <glaze/json/generic.hpp>
#include <string>
#include <utility>

#include "Simo/compiler/Compiler.h"

namespace Simo::Parameter {

/// Base class for any Parameter
class Parameter {
 public:
  Parameter() = default;

  virtual ~Parameter() = default;

  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS()

  [[nodiscard]] boost::typeindex::type_index type() const {
    return boost::typeindex::type_id_runtime(*this);
  }

  [[nodiscard]] virtual bool validate() const = 0;

  [[nodiscard]] virtual std::unique_ptr<Parameter> clone() const = 0;

  [[nodiscard]]
  bool has_value() const {
    return has_value_;
  }

  template <typename Self>
  Self& has_value(this Self& self, const bool new_val) {
    self.has_value_ = new_val;
    return self;
  }

  [[nodiscard]]
  virtual std::expected<Parameter*, std::string> value_from_generic(
      const glz::generic_u64& glz_value) = 0;

 protected:
  bool has_value_ = false;
};

/// A Parameter with a type
template <typename T>
class ParameterTyped : public Parameter {
 public:
  ~ParameterTyped() override = default;
  BOOST_TYPE_INDEX_REGISTER_RUNTIME_CLASS(Parameter);

  using Validator = std::function<bool(const T&)>;

  ParameterTyped() = default;

  explicit ParameterTyped(const T& value) : value_(value) { has_value_ = true; }

  explicit ParameterTyped(const T&& value) : value_(value) {
    has_value_ = true;
  }

  ParameterTyped& validator(Validator validator) {
    validator_ = std::move(validator);
    return *this;
  }

  ParameterTyped& value(const T& value) {
    value_ = value;
    has_value_ = true;
    return *this;
  }

  [[nodiscard]]
  std::expected<Parameter*, std::string> value_from_generic(
      const glz::generic_u64& glz_value) override {
    if (auto ec = glz::read_json(value_, glz_value)) {
      return std::unexpected(glz::format_error(ec));
    }
    has_value_ = true;
    return this;
  }

  [[nodiscard]] T value() const { return value_; }

  [[nodiscard]] bool validate() const override {
    if (!has_value_) {
      return false;
    }
    if (validator_ == nullptr) {
      return true;
    }
    return validator_(value_);
  }

  [[nodiscard]] std::unique_ptr<Parameter> clone() const override {
    return std::make_unique<ParameterTyped>(value_);
  }

 protected:
  Validator validator_{};
  T value_{};
};
}  // namespace Simo::Parameter
#endif  // SIMO_PARAMETER_HH
