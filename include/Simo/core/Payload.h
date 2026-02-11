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

#ifndef SIMO_PAYLOAD_HH
#define SIMO_PAYLOAD_HH

#include <boost/pool/pool.hpp>

namespace Simo::Payload {
template <typename T>
class Pool {
 public:
  template <typename... Args>
  T* malloc(Args... args) {
    return new (p.malloc()) T(std::forward<Args>(args)...);
  }

  void free(T* el) { p.free(el); }

 private:
  boost::object_pool<T> p;
};
}  // namespace Simo::Payload
#endif  // SIMO_PAYLOAD_HH