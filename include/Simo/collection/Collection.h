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

#define SIMO_STRINGIFY_IMPL(x) #x
#define SIMO_STRINGIFY(x) SIMO_STRINGIFY_IMPL(x)

#define SIMO_COLLECTION_FUNCTION_NAME_SYMBOL SIMO_COLLECTION_FUNCTION
#define SIMO_COLLECTION_FUNCTION_NAME_SYMBOL_STR \
  SIMO_STRINGIFY(SIMO_COLLECTION_FUNCTION_NAME_SYMBOL)

#define SIMO_COLLECTION_FUNCTION_NAME_DEFAULT simo_get_collection
#define SIMO_COLLECTION_FUNCTION_NAME_DEFAULT_STR \
  SIMO_STRINGIFY(SIMO_COLLECTION_FUNCTION_NAME_DEFAULT)

#define SIMO_COLLECTION_SECTION_STR ".Simo.collection"

#if defined(__APPLE__)
#define SIMO_COLLECTION_SECTION "__TEXT," SIMO_COLLECTION_SECTION_STR
#else
#define SIMO_COLLECTION_SECTION SIMO_COLLECTION_SECTION_STR
#endif

#define SIMO_COLLECTION_DECLARATION_NAME(name)                               \
  extern SIMO_PUBLIC __attribute__((used, section(SIMO_COLLECTION_SECTION))) \
  const char SIMO_COLLECTION_FUNCTION_NAME_SYMBOL[] = SIMO_STRINGIFY(name);

#define SIMO_COLLECTION_DECLARATION \
  SIMO_COLLECTION_DECLARATION_NAME(SIMO_COLLECTION_FUNCTION_NAME_DEFAULT)

namespace Simo {
class Module;
class Parameters;

namespace SIMO_PUBLIC Collections {
struct Factory {
  [[nodiscard]] std::string_view get_name() const noexcept;

  [[nodiscard]] Module* get_module_unsafe() const;

  [[nodiscard]] Parameters* get_parameters_unsafe() const;

  [[nodiscard]] std::unique_ptr<Module> get_module() const;

  [[nodiscard]] std::unique_ptr<Parameters> get_parameters() const;

  const char* name;
  Module* (*get_module_function)();
  Parameters* (*get_parameters_function)();
};

struct SIMO_PUBLIC SimoCollectionVersion {
  unsigned major;
  unsigned minor;
  unsigned patch;

  bool operator==(const SimoCollectionVersion& simo_collection_version) const;
};

struct SIMO_PUBLIC SimoCollection {
  const char* name;
  SimoCollectionVersion version;
  const Factory* factory_list;
  unsigned factory_list_size;

  void check() const noexcept;

  [[nodiscard]] const Factory* get_factory(
      const std::string_view& factory_name) const noexcept;

  [[nodiscard]] std::vector<std::pair<std::string_view, const Factory*>>
  get_factory_pairs() const noexcept;
};

/// Use RAII to cleanup the associated library.
/// Collection must NOT be deallocated.
/// Class is moveable but not copyable
class SIMO_PUBLIC CollectionWithLib {
 public:
  CollectionWithLib(const std::filesystem::path& path,
                    const SimoCollection* collection,
                    void* lib_handle) noexcept;

  CollectionWithLib(const CollectionWithLib&) = delete;
  CollectionWithLib& operator=(const CollectionWithLib&) = delete;

  CollectionWithLib(CollectionWithLib&& other) noexcept;

  CollectionWithLib& operator=(CollectionWithLib&& other) noexcept;

  ~CollectionWithLib();

  [[nodiscard]] const SimoCollection* get_collection() const noexcept;

 private:
  std::filesystem::path path;
  const SimoCollection* collection;
  void* lib_handle = nullptr;
};

enum struct SIMO_PUBLIC GET_COLLECTION_ERROR : std::uint8_t {
  DLOPEN_FAIL,
  NO_FUNCTION_NAME,
  NOT_A_DIRECTORY,
  ALREADY_LOADED_LIBRARY,
  NO_SIMO_COLLECTION_SECTION,
};

struct SIMO_PUBLIC GetCollectionError {
  GET_COLLECTION_ERROR error_code;
  std::string error_message;
};

/// Get a SimoCollection from the shared object
[[nodiscard]] std::expected<CollectionWithLib, GetCollectionError> SIMO_PUBLIC
simo_get_collection(const std::filesystem::path& path_to_collection);

[[nodiscard]] std::expected<std::vector<CollectionWithLib>, GetCollectionError>
    SIMO_PUBLIC
    simo_get_collection_from_folder(const std::filesystem::path& folder_path);
}  // namespace SIMO_PUBLIC Collections
}  // namespace Simo

#endif  // SIMO_COLLECTION_HH
