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

#define BOOST_TEST_MODULE SimoParameter
#include <Simo/parameter/Parameter.h>
#include <Simo/parameter/ParameterTrie.h>

#include <memory>
#include <string>

#include "support/BoostInclude.h"

BOOST_AUTO_TEST_CASE(ParameterTyped_default_constructor_and_setters) {
  using Simo::Parameter::ParameterTyped;

  ParameterTyped<int> p;
  BOOST_CHECK_EQUAL(p.value(), 0);

  p.value(42);
  BOOST_CHECK_EQUAL(p.value(), 42);

  p.value(-7);
  BOOST_CHECK_EQUAL(p.value(), -7);
}

BOOST_AUTO_TEST_CASE(ParameterTyped_type) {
  using Simo::Parameter::Parameter;
  using Simo::Parameter::ParameterTyped;

  ParameterTyped<int> p;
  BOOST_CHECK_EQUAL(p.type().pretty_name(),
                    "Simo::Parameter::ParameterTyped<int>");
}

BOOST_AUTO_TEST_CASE(ParameterTyped_value_constructor) {
  using Simo::Parameter::ParameterTyped;

  const std::string fast{"fast"};
  ParameterTyped<std::string> p(fast);
  p.validator([](const std::string& s) { return s == "fast" || s == "slow"; });
  BOOST_CHECK_EQUAL(p.validate(), true);
  BOOST_CHECK_EQUAL(p.value(), "fast");
}

BOOST_AUTO_TEST_CASE(ParameterTyped_has_value_setter_clone_and_generic_value) {
  using Simo::Parameter::Parameter;
  using Simo::Parameter::ParameterTyped;

  ParameterTyped<int> p;
  BOOST_CHECK_EQUAL(&p.has_value(true), &p);
  BOOST_CHECK_EQUAL(p.has_value(), true);
  BOOST_CHECK_EQUAL(p.validate(), true);

  auto valid_value = glz::read_json<glz::generic_u64>("123");
  BOOST_REQUIRE(valid_value.has_value());
  auto generic_result = p.value_from_generic(*valid_value);
  BOOST_REQUIRE(generic_result.has_value());
  BOOST_CHECK_EQUAL(generic_result.value(), static_cast<Parameter*>(&p));
  BOOST_CHECK_EQUAL(p.value(), 123);

  auto clone = p.clone();
  auto* cloned_int =
      boost::typeindex::runtime_cast<ParameterTyped<int>*>(clone.get());
  BOOST_REQUIRE_NE(cloned_int, nullptr);
  BOOST_CHECK_EQUAL(cloned_int->value(), 123);
  BOOST_CHECK_EQUAL(cloned_int->has_value(), true);

  ParameterTyped<int> invalid;
  auto invalid_value = glz::read_json<glz::generic_u64>(R"("not-an-int")");
  BOOST_REQUIRE(invalid_value.has_value());
  auto invalid_result = invalid.value_from_generic(*invalid_value);
  BOOST_CHECK_EQUAL(invalid_result.has_value(), false);
}

BOOST_AUTO_TEST_CASE(ParameterTyped_has_value_false) {
  using Simo::Parameter::ParameterTyped;

  ParameterTyped<std::string> p;
  BOOST_CHECK_EQUAL(p.has_value(), false);
  const bool v_result = p.validate();
  BOOST_CHECK_EQUAL(v_result, false);
}

BOOST_AUTO_TEST_CASE(ParameterTrie_find_typed_success) {
  using Simo::Parameter::ParameterTrie;

  ParameterTrie trie;
  const auto& out = trie.add<uint32_t>("core/width", 64U);

  auto* const ptr = trie.find<uint32_t>("core/width");
  BOOST_REQUIRE_NE(ptr, nullptr);
  BOOST_CHECK_EQUAL(&out, ptr);
  BOOST_CHECK_EQUAL(ptr->value(), 64U);
}

BOOST_AUTO_TEST_CASE(ParameterTrie_typed_find_wrong_type_and_missing_path) {
  using Simo::Parameter::ParameterTrie;
  using Simo::Parameter::ParameterTyped;

  ParameterTrie trie;
  const auto& _ = trie.add<uint32_t>("core/width", 64);

  auto* const wrong_type = trie.find<std::string>("core/width");
  BOOST_CHECK_EQUAL(wrong_type, nullptr);

  auto* const wrong_name_typed = trie.find<uint32_t>("core/height");
  BOOST_CHECK_EQUAL(wrong_name_typed, nullptr);

  auto* const wrong_name = trie.find("core/height");
  BOOST_CHECK_EQUAL(wrong_name, nullptr);
}

BOOST_AUTO_TEST_CASE(ParameterTrie_overwrite_value_with_different_type) {
  using Simo::Parameter::ParameterTrie;

  ParameterTrie trie;
  using Simo::Parameter::ParameterTyped;
  const auto& uint32_param = trie.add<uint32_t>("abc/def", 42);

  auto* first = trie.find<uint32_t>("abc/def");
  BOOST_REQUIRE_NE(first, nullptr);
  BOOST_CHECK_EQUAL(&uint32_param, first);
  BOOST_CHECK_EQUAL(first->value(), 42U);

  const auto& string_param = trie.add<std::string>("abc/def", "hello");
  auto* second = trie.find<std::string>("abc/def");
  BOOST_REQUIRE_NE(second, nullptr);
  BOOST_CHECK_EQUAL(&string_param, second);
  BOOST_CHECK_EQUAL(second->value(), "hello");
}

BOOST_AUTO_TEST_CASE(ParameterTrie_root_value_and_empty_lookup) {
  using Simo::Parameter::ParameterTrie;

  ParameterTrie trie;
  const auto& root_param = trie.add<bool>("", true);

  auto* root_find_ptr = trie.find<bool>("");
  BOOST_REQUIRE_NE(root_find_ptr, nullptr);
  BOOST_CHECK_EQUAL(root_find_ptr, &root_param);
  BOOST_CHECK_EQUAL(root_find_ptr->value(), true);

  auto* root_untyped = trie.find("");
  BOOST_REQUIRE_NE(root_untyped, nullptr);
  BOOST_CHECK_EQUAL(root_untyped, &root_param);
}

BOOST_AUTO_TEST_CASE(ParameterTrie_subtrie_lookup_branches) {
  using Simo::Parameter::ParameterTrie;

  ParameterTrie trie;
  const auto& value_param = trie.add<double>("tree/node/value", 3.14);

  const auto* missing = trie.get_subtrie("does/not/exist");
  BOOST_CHECK_EQUAL(missing, nullptr);

  const auto* existing_branch = trie.get_subtrie("tree");
  BOOST_REQUIRE_NE(existing_branch, nullptr);
  auto* const nested_value = existing_branch->find<double>("node/value");
  BOOST_REQUIRE_NE(nested_value, nullptr);
  BOOST_CHECK_EQUAL(nested_value, &value_param);
  BOOST_CHECK_CLOSE(nested_value->value(), 3.14, 0.0001);
}
