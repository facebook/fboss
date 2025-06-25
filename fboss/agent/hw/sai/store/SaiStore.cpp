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
        using ObjectTraits =
            typename std::decay_t<decltype(store)>::ObjectTraits;
        const folly::dynamic* adapterKeys = adapterKeysJson
            ? adapterKeysJson->get_ptr(store.objectTypeName())
            : nullptr;
        const folly::dynamic* adapterHostKeys = adapterKeys2AdapterHostKeyJson
            ? adapterKeys2AdapterHostKeyJson->get_ptr(store.objectTypeName())
            : nullptr;
        // Refer to D75845886 for details
        // In the adapterKey2AdapterHostKey map in warm boot file,
        // Pre D75845886, adapterHostKey for nhop-group is a simple list of
        // nhop members.
        // Post D75845886, adapterHostKey changes to a pair of <members, mode>
        //
        // To support warmboot downgrade, both formats will be written to
        //  - Existing "nhop-group" holds the old members only key
        //  - Temporary "nhop-group-temporary" holds the <members, mode> pair
        // More details in SaiStore::adapterKeys2AdapterHostKeysFollyDynamic
        //
        // Until all SAI switches upgrade to the 2nd format above, both maps
        // with above keys will be written to the warm boot file
        //
        // For the 1st warmboot to this image, "nhop-group-temporary" will be
        // absent and below code would be a NOP.
        // If it is present, prefer the new map with the new format
        if (ObjectTraits::ObjectType == SAI_OBJECT_TYPE_NEXT_HOP_GROUP) {
          const folly::dynamic* newAdapterHostKeys =
              adapterKeys2AdapterHostKeyJson
              ? adapterKeys2AdapterHostKeyJson->get_ptr(kNhopGroupWithModeName)
              : nullptr;
          if (newAdapterHostKeys) {
            adapterHostKeys = newAdapterHostKeys;
          }
        }

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
          // Refer to D75845886 for details
          if (ObjectTraits::ObjectType == SAI_OBJECT_TYPE_NEXT_HOP_GROUP) {
            /* For backward compatibility, existing map "nhop-group" is updated
             * in old format
             *
             * "nhop-group" : {
             *   "nhopGroup0" : [ memberList ],
             *   "nhopGroup1" : [ memberList ],
             * }
             */
            // Just get the members from the new format and ignore mode
            folly::dynamic temporaryJson = folly::dynamic::object;
            for (const auto& object : json.items()) {
              temporaryJson[object.first] = object.second[AttributeName<
                  SaiNextHopGroupTraits::Attributes::NextHopMemberList>::value];
            }
            storeJson[store.objectTypeName()] = temporaryJson;

            // TODO(ravi) Remove this new map and use the new format in the
            // above existing nhop-group map
            /* New format goes into a new map called "nhop-group-temporary"
             * "nhop-group-temporary" : {
             *   "nhopGroup0" : {
             *     "NextHopMemberList" : [ memberList ],
             *     "Mode" : 2,
             *   },
             *   "nhopGroup1" : {
             *     "NextHopMemberList" : [ memberList ],
             *     "Mode" : 4,
             *   },
             * }
             */
            storeJson[kNhopGroupWithModeName] = json;
          } else {
            storeJson[store.objectTypeName()] = json;
          }
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
