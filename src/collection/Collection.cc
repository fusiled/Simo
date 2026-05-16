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
#include "Simo/collection/Collection.h"

#include <dlfcn.h>

#include <expected>
#include <filesystem>
#include <fstream>

namespace Simo::Collections {
static_assert(std::is_standard_layout_v<Factory>,
              "Factory must be standard layout");
static_assert(std::is_standard_layout_v<SimoCollection>,
              "Factory must be standard layout");
std::string_view Factory::get_name() const noexcept { return {name}; }

Module* Factory::get_module_unsafe() const { return get_module_function(); }

Parameters* Factory::get_parameters_unsafe() const {
  return get_parameters_function();
}

std::unique_ptr<Module> Factory::get_module() const {
  return std::unique_ptr<Module>(get_module_function());
}

std::unique_ptr<Parameters> Factory::get_parameters() const {
  return std::unique_ptr<Parameters>(get_parameters_function());
}

bool SimoCollectionVersion::operator==(
    const SimoCollectionVersion& simo_collection_version) const {
  return major == simo_collection_version.major &&
         minor == simo_collection_version.minor &&
         patch == simo_collection_version.patch;
}

void SimoCollection::check() const noexcept {
  // TODO implement checks on no duplicate names in factory list
}

const Factory* SimoCollection::get_factory(
    const std::string_view& factory_name) const noexcept {
  for (unsigned i = 0; i < factory_list_size; ++i) {
    if (factory_list[i].get_name() == factory_name) {
      return &factory_list[i];
    }
  }
  return nullptr;
}

std::vector<std::pair<std::string_view, const Factory*>>
SimoCollection::get_factory_pairs() const noexcept {
  std::vector<std::pair<std::string_view, const Factory*>> factory_pairs;
  factory_pairs.reserve(factory_list_size);
  for (unsigned i = 0; i < factory_list_size; ++i) {
    factory_pairs.emplace_back(factory_list[i].get_name(), factory_list + i);
  }
  return factory_pairs;
}

CollectionWithLib::CollectionWithLib(const std::filesystem::path& path,
                                     const SimoCollection* collection,
                                     void* lib_handle) noexcept
    : path(path), collection(collection), lib_handle(lib_handle) {}

CollectionWithLib::CollectionWithLib(CollectionWithLib&& other) noexcept
    : path(std::move(other.path)),
      collection(other.collection),
      lib_handle(std::exchange(other.lib_handle, nullptr)) {}

CollectionWithLib& CollectionWithLib::operator=(
    CollectionWithLib&& other) noexcept {
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

CollectionWithLib::~CollectionWithLib() {
  if (lib_handle != nullptr) {
    dlclose(lib_handle);
  }
}

const SimoCollection* CollectionWithLib::get_collection() const noexcept {
  return collection;
}

std::expected<CollectionWithLib, GetCollectionError> simo_get_collection(
    const std::filesystem::path& path_to_collection) {
  // Probe to see if the library has already loaded.
  void* probe_lib = dlopen(path_to_collection.c_str(), RTLD_LAZY | RTLD_NOLOAD);
  if (probe_lib != nullptr) {
    // Already loaded. No need to do anything
    dlclose(probe_lib);
    return std::unexpected<GetCollectionError>(
        {.error_code = GET_COLLECTION_ERROR::ALREADY_LOADED_LIBRARY,
         .error_message = ""});
  }
  void* lib = dlopen(path_to_collection.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (lib == nullptr) {
    return std::unexpected<GetCollectionError>(
        {.error_code = GET_COLLECTION_ERROR::DLOPEN_FAIL,
         .error_message = std::string(dlerror())});
  }
  dlerror();  // Clear any existing error
  const char* collection_function_name = reinterpret_cast<const char*>(
      dlsym(lib, SIMO_COLLECTION_FUNCTION_NAME_SYMBOL));
  const char* function_name = collection_function_name != nullptr
                                  ? collection_function_name
                                  : SIMO_COLLECTION_FUNCTION_NAME_DEFAULT;
  // If SIMO_COLLECTION_FUNCTION_NAME_DEFAULT is not found, fall-back to default
  // value
  dlerror();
  const auto get_simo_collection_fun =
      reinterpret_cast<const SimoCollection* (*)()>(dlsym(lib, function_name));
  if (const char* err = dlerror()) {
    dlclose(lib);
    return std::unexpected<GetCollectionError>(
        {.error_code = GET_COLLECTION_ERROR::NO_FUNCTION_NAME,
         .error_message = std::string(err)});
  }
  dlerror();
  return CollectionWithLib(path_to_collection, get_simo_collection_fun(), lib);
}

std::expected<std::vector<CollectionWithLib>, GetCollectionError>
simo_get_collection_from_folder(const std::filesystem::path& folder_path) {
  namespace Fs = std::filesystem;
  if (!std::filesystem::is_directory(folder_path)) {
    GetCollectionError error{GET_COLLECTION_ERROR::NOT_A_DIRECTORY, ""};
    return std::unexpected(error);
  }
  std::vector<GetCollectionError> errors;
  std::vector<CollectionWithLib> collections;
  for (const auto& entry : Fs::directory_iterator(folder_path)) {
    auto load_result = simo_get_collection(entry.path());
    if (!load_result) {
      // TODO need a better logic to detect errors instead of ignoring them!
      // Like figure out if the opening file is a valid shared object
      // errors.push_back(load_result.error());
      continue;
    }
    collections.emplace_back(std::move(load_result.value()));
  }
  return collections;
}
}  // namespace Simo::Collections