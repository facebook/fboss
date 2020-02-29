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

#include "fboss/agent/hw/sai/api/LoggingUtil.h"

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

void SaiStore::reload() {
  tupleForEach([](auto& store) { store.reload(); }, stores_);
}

void SaiStore::release() {
  tupleForEach([](auto& store) { store.release(); }, stores_);
}

folly::dynamic SaiStore::adapterKeysFollyDynamic() const {
  folly::dynamic adapterKeys = folly::dynamic::object;
  tupleForEach(
      [&adapterKeys](auto& store) {
        using ObjectTraits =
            typename std::remove_reference_t<decltype(store)>::ObjectTraits;
        if constexpr (AdapterKeyIsObjectId<ObjectTraits>::value) {
          auto apiName = saiObjectTypeToString(ObjectTraits::ObjectType);
          adapterKeys[apiName] = store.adapterKeysFollyDynamic();
        }
      },
      stores_);
  return adapterKeys;
}

} // namespace facebook::fboss
