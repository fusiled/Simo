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

#ifndef SIMO_COLLECTION_HH
#define SIMO_COLLECTION_HH
#include <dlfcn.h>

#include <expected>
#include <filesystem>
#include <vector>

#include "Simo/module/Module.h"

#define SIMO_COLLECTION_FUNCTION_NAME_SYMBOL "SIMO_COLLECTION_FUNCTION"

#define SIMO_COLLECTION_FUNCTION_NAME_DEFAULT "simo_get_collection"

namespace Simo {
class Module;
class Parameters;

namespace Collections {
struct Factory {
  [[nodiscard]] std::string_view get_name() const noexcept { return {name}; }

  [[nodiscard]] Module* get_module_unsafe() const {
    return get_module_function();
  }

  [[nodiscard]] Parameters* get_parameters_unsafe() const {
    return get_parameters_function();
  }

  [[nodiscard]] std::unique_ptr<Module> get_module() const {
    return std::unique_ptr<Module>(get_module_function());
  }

  [[nodiscard]] std::unique_ptr<Parameters> get_parameters() const {
    return std::unique_ptr<Parameters>(get_parameters_function());
  }

  const char* name;
  Module* (*get_module_function)();
  Parameters* (*get_parameters_function)();
};

struct SimoCollectionVersion {
  unsigned major;
  unsigned minor;
  unsigned patch;

  bool operator==(const SimoCollectionVersion& simo_collection_version) const {
    return major == simo_collection_version.major &&
           minor == simo_collection_version.minor &&
           patch == simo_collection_version.patch;
  }
};

struct SimoCollection {
  const char* name;
  SimoCollectionVersion version;
  const Factory* factory_list;
  unsigned factory_list_size;

  void check() const noexcept {
    // TODO implement checks on no duplicate names in factory list
  }

  [[nodiscard]] const Factory* get_factory(
      const std::string_view& factory_name) const noexcept {
    for (unsigned i = 0; i < factory_list_size; ++i) {
      if (factory_list[i].get_name() == factory_name) {
        return &factory_list[i];
      }
    }
    return nullptr;
  }

  std::vector<std::pair<std::string_view, const Factory*>> get_factory_pairs() const noexcept {
    std::vector<std::pair<std::string_view, const Factory*>> factory_pairs;
    factory_pairs.reserve(factory_list_size);
    for (unsigned i = 0; i < factory_list_size; ++i) {
      factory_pairs.emplace_back(factory_list[i].get_name(), factory_list+i);
    }
    return factory_pairs;
  }
};

/// Use RAII to cleanup the associated library.
/// Collection must NOT be deallocated.
/// Class is moveable but not copyable
class CollectionWithLib {
 public:
  CollectionWithLib(const std::filesystem::path& path,
                    const SimoCollection* collection, void* lib_handle) noexcept
      : path(path), collection(collection), lib_handle(lib_handle) {}

  CollectionWithLib(const CollectionWithLib&) = delete;
  CollectionWithLib& operator=(const CollectionWithLib&) = delete;

  CollectionWithLib(CollectionWithLib&& other) noexcept
      : path(other.path),
        collection(other.collection),
        lib_handle(std::exchange(other.lib_handle, nullptr)) {}

  CollectionWithLib& operator=(CollectionWithLib&& other) noexcept {
    if (this != &other) {
      collection = std::exchange(other.collection, nullptr);
      if (lib_handle != nullptr) {
        dlclose(lib_handle);
      }
      lib_handle = std::exchange(other.lib_handle, nullptr);
    }
    path = other.path;
    other.path = "";
    return *this;
  }

  ~CollectionWithLib() {
    if (lib_handle != nullptr) {
      dlclose(lib_handle);
    }
  }

  const SimoCollection* get_collection() const noexcept { return collection; }

 private:
  std::filesystem::path path;
  const SimoCollection* collection;
  void* lib_handle = nullptr;
};

enum struct GET_COLLECTION_ERROR : std::uint8_t {
  DLOPEN_FAIL,
  NO_FUNCTION_NAME,
  NOT_A_DIRECTORY,
  ALREADY_LOADED_LIBRARY
};

struct GetCollectionError {
  GET_COLLECTION_ERROR error_code;
  std::string error_message;
};

/// Get a SimoCollection from the shared object
[[nodiscard]] std::expected<CollectionWithLib, GetCollectionError> SIMO_PUBLIC
simo_get_collection(const std::filesystem::path& path_to_collection);

[[nodiscard]] std::expected<std::vector<CollectionWithLib>, GetCollectionError>
    SIMO_PUBLIC
    simo_get_collection_from_folder(const std::filesystem::path& folder_path);
}  // namespace Collections
}  // namespace Simo

#endif  // SIMO_COLLECTION_HH
