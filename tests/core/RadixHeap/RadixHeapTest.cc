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

#define BOOST_TEST_MODULE RadixHeap
#include <Simo/core/internal/RadixHeap.h>

#include <boost/test/unit_test.hpp>

using Simo::Internal::RadixHeap;

BOOST_AUTO_TEST_CASE(pushElements) {
  RadixHeap heap;

  BOOST_CHECK_EQUAL(heap.empty(), true);

  // Fill the heap
  heap.push(0);
  BOOST_CHECK_EQUAL(heap.peek(), 0);
  BOOST_CHECK_EQUAL(heap.size(), 1);
  BOOST_CHECK_EQUAL(heap.empty(), false);
  heap.push(0);
  BOOST_CHECK_EQUAL(heap.peek(), 0);
  BOOST_CHECK_EQUAL(heap.size(), 2);
  heap.push(std::numeric_limits<uint64_t>::max());
  BOOST_CHECK_EQUAL(heap.peek(), 0);
  BOOST_CHECK_EQUAL(heap.size(), 3);

  // Empty the heap
  const uint64_t v0 = heap.peek();
  heap.pop();
  BOOST_CHECK_EQUAL(v0, 0);
  BOOST_CHECK_EQUAL(heap.size(), 2);

  const uint64_t v1 = heap.peek();
  heap.pop();
  BOOST_CHECK_EQUAL(v1, 0);
  BOOST_CHECK_EQUAL(heap.size(), 1);

  const uint64_t v2 = heap.peek();
  heap.pop();
  BOOST_CHECK_EQUAL(v2, std::numeric_limits<uint64_t>::max());
  BOOST_CHECK_EQUAL(heap.size(), 0);
  BOOST_CHECK_EQUAL(heap.empty(), true);

  // Add one other element
  heap.push(0);
  BOOST_CHECK_EQUAL(heap.peek(), 0);
  BOOST_CHECK_EQUAL(heap.size(), 1);
}

BOOST_AUTO_TEST_CASE(pushElementReorder) {
  RadixHeap heap;

  // Fill the heap
  heap.push(0);
  BOOST_CHECK_EQUAL(heap.peek(), 0);
  BOOST_CHECK_EQUAL(heap.size(), 1);
  BOOST_CHECK_EQUAL(heap.empty(), false);
  heap.push(1000);
  BOOST_CHECK_EQUAL(heap.peek(), 0);
  BOOST_CHECK_EQUAL(heap.size(), 2);
  heap.push(2000);
  BOOST_CHECK_EQUAL(heap.peek(), 0);
  BOOST_CHECK_EQUAL(heap.size(), 3);

  // Empty the heap
  const uint64_t v0 = heap.peek();
  heap.pop();
  BOOST_CHECK_EQUAL(v0, 0);
  BOOST_CHECK_EQUAL(heap.size(), 2);

  const uint64_t v1 = heap.peek();
  heap.pop();
  BOOST_CHECK_EQUAL(v1, 1000);
  BOOST_CHECK_EQUAL(heap.size(), 1);

  const uint64_t v2 = heap.peek();
  heap.pop();
  BOOST_CHECK_EQUAL(v2, 2000);
  BOOST_CHECK_EQUAL(heap.size(), 0);
  BOOST_CHECK_EQUAL(heap.empty(), true);

  // Add one other element
  heap.push(0);
  BOOST_CHECK_EQUAL(heap.peek(), 0);
  BOOST_CHECK_EQUAL(heap.size(), 1);
}