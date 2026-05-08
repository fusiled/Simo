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

#ifndef SIMO_STATOUTSTREAM_HH
#define SIMO_STATOUTSTREAM_HH

namespace Simo::Statistics {

class StatMapper;
class Statistic;

/// Base class to dump statistics
class StatOutStreamInterface {
 public:
  virtual ~StatOutStreamInterface() = default;

  template <typename Self>
  Self& operator<<(this Self&& self, const Statistic& s) {
    return self << s;
  }

  template <typename Self>
  void reset(this Self&& self) {
    self.reset();
  }
  template <typename Self>
  void generate(this Self&& self) {
    self.generate();
  }
};

/// Basic implementation to dump statistics as a json
class StatOutStream : public StatOutStreamInterface {
 public:
  ~StatOutStream() override = default;

  StatOutStream() : array(glz::generic::array_t{}) {}

  std::filesystem::path output_path() const { return output_path_; }
  void output_path(std::filesystem::path path) {
    output_path_ = std::move(path);
  }

  StatOutStream& operator<<(const Statistic& s) {
    array.get<glz::generic::array_t>().emplace_back(s.to_json());
    return *this;
  }

  void reset() { array = glz::generic::array_t{}; }

  void generate() {
    std::string buffer;
    // TODO hanndle errors
    auto _ = glz::write_file_json(array, "output.json", buffer);
  }

 private:
  std::filesystem::path output_path_;
  glz::generic array;
};
}  // namespace Simo::Statistics

#endif  // SIMO_STATOUTSTREAM_HH