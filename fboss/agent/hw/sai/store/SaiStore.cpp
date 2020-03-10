/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/store/SaiStore.h"

#include <folly/Singleton.h>

namespace {
struct singleton_tag_type {};
} // namespace

using facebook::fboss::SaiStore;
static folly::Singleton<SaiStore, singleton_tag_type> saiStoreSingleton{};
std::shared_ptr<SaiStore> SaiStore::getInstance() {
  return saiStoreSingleton.try_get();
}

namespace facebook::fboss {

SaiStore::SaiStore() {}

SaiStore::SaiStore(sai_object_id_t switchId) {
  setSwitchId(switchId);
}

void SaiStore::setSwitchId(sai_object_id_t switchId) {
  switchId_ = switchId;
  tupleForEach(
      [switchId](auto& store) { store.setSwitchId(switchId); }, stores_);
}

void SaiStore::reload(const folly::dynamic* adapterKeysJson) {
  tupleForEach(
      [adapterKeysJson](auto& store) {
        store.reload(
            adapterKeysJson ? &((*adapterKeysJson)[store.objectTypeName()])
                            : nullptr);
      },
      stores_);
}

void SaiStore::release() {
  tupleForEach([](auto& store) { store.release(); }, stores_);
}

folly::dynamic SaiStore::adapterKeysFollyDynamic() const {
  folly::dynamic adapterKeys = folly::dynamic::object;
  tupleForEach(
      [&adapterKeys](auto& store) {
        using ObjectTraits =
            typename std::decay_t<decltype(store)>::ObjectTraits;
        auto objName = store.objectTypeName();
        if constexpr (AdapterKeyIsObjectId<ObjectTraits>::value) {
          auto aitr = adapterKeys.find(objName);
          if (aitr == adapterKeys.items().end()) {
            adapterKeys[objName] = folly::dynamic::array;
            aitr = adapterKeys.find(objName);
          }
          for (auto key : store.adapterKeysFollyDynamic()) {
            aitr->second.push_back(key);
          }
        } else {
          // TODO - start serializing non OID adapter keys
          adapterKeys[objName] = folly::dynamic::array;
        }
      },
      stores_);
  return adapterKeys;
}

void SaiStore::exitForWarmBoot() {
  tupleForEach([](auto& store) { store.exitForWarmBoot(); }, stores_);
}

} // namespace facebook::fboss
