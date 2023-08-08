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

namespace facebook::fboss {

SaiStore::SaiStore() {}

SaiStore::SaiStore(sai_object_id_t switchId) {
  setSwitchId(switchId);
}

void SaiStore::setSwitchId(sai_object_id_t switchId) {
  saiSwitchId_ = switchId;
  tupleForEach(
      [switchId](auto& store) { store.setSwitchId(switchId); }, stores_);
}

void SaiStore::reload(
    const folly::dynamic* adapterKeysJson,
    const folly::dynamic* adapterKeys2AdapterHostKeyJson) {
  tupleForEach(
      [adapterKeysJson, adapterKeys2AdapterHostKeyJson](auto& store) {
        const folly::dynamic* adapterKeys = adapterKeysJson
            ? adapterKeysJson->get_ptr(store.objectTypeName())
            : nullptr;
        const folly::dynamic* adapterHostKeys = adapterKeys2AdapterHostKeyJson
            ? adapterKeys2AdapterHostKeyJson->get_ptr(store.objectTypeName())
            : nullptr;

        store.reload(adapterKeys, adapterHostKeys);
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
        auto objName = store.objectTypeName();
        auto aitr = adapterKeys.find(objName);
        if (aitr == adapterKeys.items().end()) {
          adapterKeys[objName] = folly::dynamic::array;
          aitr = adapterKeys.find(objName);
        }
        for (auto key : store.adapterKeysFollyDynamic()) {
          aitr->second.push_back(key);
        }
      },
      stores_);
  return adapterKeys;
}

void SaiStore::exitForWarmBoot() {
  tupleForEach([](auto& store) { store.exitForWarmBoot(); }, stores_);
}

folly::dynamic SaiStore::adapterKeys2AdapterHostKeysFollyDynamic() const {
  folly::dynamic storeJson = folly::dynamic::object;

  tupleForEach(
      [&storeJson](auto& store) {
        using ObjectTraits =
            typename std::decay_t<decltype(store)>::ObjectTraits;

        if constexpr (!AdapterHostKeyWarmbootRecoverable<ObjectTraits>::value) {
          folly::dynamic json = folly::dynamic::object;
          const auto& objects = store.objects();
          for (auto iter : objects) {
            auto object = iter.second.lock();
            json[folly::to<std::string>(object->adapterKey())] =
                object->adapterHostKeyToFollyDynamic();
          }
          storeJson[store.objectTypeName()] = json;
        }
      },
      stores_);

  return storeJson;
}

std::string SaiStore::storeStr(sai_object_type_t objType) const {
  std::string output;
  tupleForEach(
      [objType, &output](const auto& store) {
        using StoreType = std::decay_t<decltype(store)>;
        using ObjectTraits = typename StoreType::ObjectTraits;
        if (objType == ObjectTraits::ObjectType) {
          output += fmt::format("{}\n", store);
        }
      },
      stores_);
  if (!output.size()) {
    throw FbossError("Object type {}, not found ", objType);
  }
  return output;
}

void SaiStore::checkUnexpectedUnclaimedWarmbootHandles() const {
  bool hasUnexpectedUnclaimedWarmbootHandles = false;
  tupleForEach(
      [&hasUnexpectedUnclaimedWarmbootHandles](const auto& store) {
        hasUnexpectedUnclaimedWarmbootHandles |=
            store.hasUnexpectedUnclaimedWarmbootHandles();
      },
      stores_);
  if (hasUnexpectedUnclaimedWarmbootHandles) {
    XLOG(FATAL) << "unclaimed objects found in store";
  }
}

void SaiStore::printWarmbootHandles() const {
  tupleForEach(
      [](const auto& store) { store.printWarmBootHandles(); }, stores_);
}

void SaiStore::removeUnexpectedUnclaimedWarmbootHandles() {
  tupleForEach(
      [](auto& store) { store.removeUnexpectedUnclaimedWarmbootHandles(); },
      stores_);
}

} // namespace facebook::fboss
