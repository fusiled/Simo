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
#include <fcntl.h>
#include <unistd.h>

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
#include <string_view>
#include <vector>

#if defined(__APPLE__)
#include <mach-o/loader.h>
#elif defined(__linux__)
#include <elf.h>
#else
#error \
    "Unsupported platform: this source supports only macOS Mach-O and Linux ELF."
#endif

#include <expected>
#include <filesystem>
#include <fstream>

class unique_fd {
 public:
  explicit unique_fd(int fd = -1) noexcept : fd_(fd) {}

  ~unique_fd() {
    if (fd_ >= 0) {
      close(fd_);
    }
  }

  unique_fd(const unique_fd&) = delete;
  unique_fd& operator=(const unique_fd&) = delete;

  unique_fd(unique_fd&& other) noexcept : fd_(other.fd_) { other.fd_ = -1; }

  unique_fd& operator=(unique_fd&& other) noexcept {
    if (this != &other) {
      if (fd_ >= 0) {
        close(fd_);
      }

      fd_ = other.fd_;
      other.fd_ = -1;
    }

    return *this;
  }

  [[nodiscard]] int get() const noexcept { return fd_; }

  [[nodiscard]] explicit operator bool() const noexcept { return fd_ >= 0; }

 private:
  int fd_;
};

bool read_exact(int fd, void* buffer, std::size_t size, off_t offset) {
  auto* out = static_cast<std::byte*>(buffer);
  std::size_t total = 0;

  while (total < size) {
    const ssize_t n = pread(fd, out + total, size - total, offset + total);

    if (n < 0) {
      if (errno == EINTR) {
        continue;
      }

      return false;
    }

    if (n == 0) {
      return false;
    }

    total += static_cast<std::size_t>(n);
  }

  return true;
}

#if defined(__linux__)

bool elf_string_at_equals(const std::vector<char>& table, std::uint32_t offset,
                          std::string_view expected) {
  if (offset >= table.size()) {
    return false;
  }

  const char* section_name = table.data() + offset;
  const std::size_t remaining = table.size() - offset;

  return remaining > expected.size() &&
         std::memcmp(section_name, expected.data(), expected.size()) == 0 &&
         section_name[expected.size()] == '\0';
}

bool object_file_has_section_named_impl(int fd, std::string_view section_name) {
  unsigned char ident[EI_NIDENT]{};

  if (!read_exact(fd, ident, sizeof(ident), 0)) {
    return false;
  }

  if (ident[EI_MAG0] != ELFMAG0 || ident[EI_MAG1] != ELFMAG1 ||
      ident[EI_MAG2] != ELFMAG2 || ident[EI_MAG3] != ELFMAG3 ||
      ident[EI_CLASS] != ELFCLASS64) {
    return false;
  }

  Elf64_Ehdr ehdr{};
  if (!read_exact(fd, &ehdr, sizeof(ehdr), 0)) {
    return false;
  }

  if (ehdr.e_shoff == 0 || ehdr.e_shnum == 0 || ehdr.e_shstrndx == SHN_UNDEF ||
      ehdr.e_shstrndx >= ehdr.e_shnum) {
    return false;
  }

  std::vector<Elf64_Shdr> sections(ehdr.e_shnum);

  if (!read_exact(fd, sections.data(), sections.size() * sizeof(Elf64_Shdr),
                  static_cast<off_t>(ehdr.e_shoff))) {
    return false;
  }

  const Elf64_Shdr& section_name_table = sections[ehdr.e_shstrndx];

  if (section_name_table.sh_size == 0) {
    return false;
  }

  std::vector<char> names(section_name_table.sh_size);

  if (!read_exact(fd, names.data(), names.size(),
                  static_cast<off_t>(section_name_table.sh_offset))) {
    return false;
  }

  for (const Elf64_Shdr& section : sections) {
    if (elf_string_at_equals(names, section.sh_name, section_name)) {
      return true;
    }
  }

  return false;
}

#elif defined(__APPLE__)

bool macho_fixed_name_equals(const char raw_name[16],
                             std::string_view expected) {
  if (expected.size() > 16) {
    return false;
  }

  const std::size_t raw_len = strnlen(raw_name, 16);

  if (raw_len != expected.size()) {
    return false;
  }

  return std::memcmp(raw_name, expected.data(), expected.size()) == 0;
}

bool object_file_has_section_named_impl(int fd, std::string_view section_name) {
  mach_header_64 header{};

  if (!read_exact(fd, &header, sizeof(header), 0)) {
    return false;
  }

  if (header.magic != MH_MAGIC_64) {
    return false;
  }

  off_t command_offset = static_cast<off_t>(sizeof(mach_header_64));

  for (std::uint32_t i = 0; i < header.ncmds; ++i) {
    load_command lc{};

    if (!read_exact(fd, &lc, sizeof(lc), command_offset)) {
      return false;
    }

    if (lc.cmdsize < sizeof(load_command)) {
      return false;
    }

    if (lc.cmd == LC_SEGMENT_64) {
      segment_command_64 segment{};

      if (lc.cmdsize < sizeof(segment_command_64)) {
        return false;
      }

      if (!read_exact(fd, &segment, sizeof(segment), command_offset)) {
        return false;
      }

      const off_t first_section_offset =
          command_offset + static_cast<off_t>(sizeof(segment_command_64));

      const std::size_t section_table_size =
          static_cast<std::size_t>(segment.nsects) * sizeof(section_64);

      const std::size_t available_size =
          lc.cmdsize - sizeof(segment_command_64);

      if (section_table_size > available_size) {
        return false;
      }

      for (std::uint32_t s = 0; s < segment.nsects; ++s) {
        section_64 section{};

        const off_t section_offset =
            first_section_offset + static_cast<off_t>(s * sizeof(section_64));

        if (!read_exact(fd, &section, sizeof(section), section_offset)) {
          return false;
        }

        if (macho_fixed_name_equals(section.sectname, section_name)) {
          return true;
        }
      }
    }

    command_offset += static_cast<off_t>(lc.cmdsize);
  }

  return false;
}

#endif

bool object_file_has_section_named(const std::filesystem::path& path,
                                   std::string_view section_name) {
  const unique_fd fd{open(path.c_str(), O_RDONLY | O_CLOEXEC)};

  if (!fd) {
    return false;
  }

  return object_file_has_section_named_impl(fd.get(), section_name);
}

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
  if (this == &other) {
    return *this;
  }
  if (lib_handle != nullptr) {
    dlclose(lib_handle);
  }
  path = std::move(other.path);
  collection = std::exchange(other.collection, nullptr);
  lib_handle = std::exchange(other.lib_handle, nullptr);
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
  // Check if section is present
  if (std::filesystem::exists(path_to_collection) &&
      !object_file_has_section_named(path_to_collection,
                                     SIMO_COLLECTION_SECTION_STR)) {
    return std::unexpected<GetCollectionError>(
        {.error_code = GET_COLLECTION_ERROR::NO_SIMO_COLLECTION_SECTION,
         .error_message = "No section "});
  }
  dlerror();
  // Probe to see if the library has already loaded.
  void* probe_lib = dlopen(path_to_collection.c_str(), RTLD_LAZY | RTLD_NOLOAD);
  if (probe_lib != nullptr) {
    // Already loaded. No need to do anything
    dlclose(probe_lib);
    return std::unexpected<GetCollectionError>(
        {.error_code = GET_COLLECTION_ERROR::ALREADY_LOADED_LIBRARY,
         .error_message = ""});
  }
  dlerror();
  void* lib = dlopen(path_to_collection.c_str(), RTLD_NOW | RTLD_LOCAL);
  if (lib == nullptr) {
    return std::unexpected<GetCollectionError>(
        {.error_code = GET_COLLECTION_ERROR::DLOPEN_FAIL,
         .error_message = std::string(dlerror())});
  }
  dlerror();  // Clear any existing error
  const char* collection_function_name = reinterpret_cast<const char*>(
      dlsym(lib, SIMO_COLLECTION_FUNCTION_NAME_SYMBOL_STR));
  // If SIMO_COLLECTION_FUNCTION_NAME_SYMBOL_STR is not found, fall-back to
  // default value
  const char* function_name = collection_function_name != nullptr
                                  ? collection_function_name
                                  : SIMO_COLLECTION_FUNCTION_NAME_DEFAULT_STR;
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
