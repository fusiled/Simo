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

#ifndef SIMO_RADIXHEAP_HH
#define SIMO_RADIXHEAP_HH

#include <algorithm>
#include <array>
#include <cstdint>
#include <limits>
#include <vector>

namespace Simo::Internal {
class RadixHeap {
 public:
  constexpr RadixHeap() : bucket_mins() {
    std::ranges::fill(bucket_mins, std::numeric_limits<uint64_t>::max());
  }

  void push(uint64_t val);

  [[nodiscard]]
  uint64_t peek() const;

  void pop();

  [[nodiscard]]
  bool empty() const;

  [[nodiscard]]
  size_t size() const;

 private:
  static size_t get_bucket_index(const uint64_t min_val, const uint64_t val) {
    const uint64_t xor_op = val ^ min_val;
    const int leading_zeroes = std::countl_zero<uint64_t>(xor_op);
    return NUM_BITS - leading_zeroes;
  }

  static constexpr size_t NUM_BITS = sizeof(uint64_t) * 8U;
  static constexpr size_t NUM_BUCKETS = NUM_BITS + 1U;

  std::array<std::vector<uint64_t>, NUM_BUCKETS> buckets{};
  std::array<uint64_t, NUM_BUCKETS> bucket_mins;
  size_t size_val{0};
};
}  // namespace Simo::Internal

#endif  // SIMO_RADIXHEAP_HH
