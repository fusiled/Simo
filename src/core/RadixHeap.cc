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
#include "../../include/Simo/core/internal/RadixHeap.h"

#include <algorithm>
#include <array>

namespace Simo::Internal {
void RadixHeap::push(uint64_t val) {
  size_val++;
  if (size_val == 1) {
    buckets[0].emplace_back(val);
    return;
  }
  const uint64_t min_val = buckets[0][0];
  const size_t bucket_idx = get_bucket_index(min_val, val);
  buckets[bucket_idx].emplace_back(val);
  bucket_mins[bucket_idx] = std::min(bucket_mins[bucket_idx], val);
}

uint64_t RadixHeap::peek() const { return buckets[0][0]; }

void RadixHeap::pop() {
  size_val--;
  buckets[0].pop_back();
  if (!buckets[0].empty()) {
    // Still have other minimum occurrencies
    return;
  }
  if (size_val == 0) {
    // Heap is now empty
    return;
  }
  // Need to rebalance heap
  auto* const non_empty_bucket_it =
      std::find_if(buckets.begin() + 1, buckets.end(),
                   [](const auto& bucket) { return !bucket.empty(); });
  auto& b = *non_empty_bucket_it;
  const size_t bucket_idx = std::distance(buckets.begin(), non_empty_bucket_it);
  const uint64_t min_val = bucket_mins[bucket_idx];
  for (const uint64_t val : b) {
    const size_t new_bucket_idx = get_bucket_index(min_val, val);
    buckets[new_bucket_idx].emplace_back(val);
    bucket_mins[new_bucket_idx] = std::min(bucket_mins[new_bucket_idx], val);
  }
  bucket_mins[bucket_idx] = std::numeric_limits<uint64_t>::max();
  b.clear();
}

bool RadixHeap::empty() const { return size_val == 0; }

size_t RadixHeap::size() const { return size_val; }
}  // namespace Simo::Internal