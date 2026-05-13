/*
 * Copyright $YEAR $USER_NAME and Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <array>

#include "Simo/collection/Collection.h"
#include "Simo/module/core/Collector.h"

extern "C" {

constexpr Simo::Collections::SimoCollectionVersion VERSION{
    .major = 0, .minor = 0, .patch = 1};

constexpr Simo::Collections::Factory COLLECTOR_FACTORY{
    .name = "collector",
    .get_module_function =
        [] {
          return static_cast<Simo::Module*>(
              new Simo::Modules::Core::Collector());
        },
    .get_parameters_function =
        [] {
          return static_cast<Simo::Parameters*>(
              new Simo::Modules::Core::Collector::Parameters());
        }};

const char* collection_name = "SimoCoreModules";
constexpr std::array FACTORY_LIST{COLLECTOR_FACTORY};
static Simo::Collections::SimoCollection collection{
    .name = collection_name,
    .version = VERSION,
    .factory_list = FACTORY_LIST.data(),
    .factory_list_size = FACTORY_LIST.size(),
};
}

namespace Simo::Modules::Core {

SIMO_PUBLIC Collections::CollectionWithLib loadCoreModules() {
  return Collections::CollectionWithLib("", &collection, nullptr);
}

}  // namespace Simo::Modules::Core
