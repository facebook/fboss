/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include "fboss/agent/hw/sai/api/AdapterKeySerializers.h"
#include "fboss/agent/hw/sai/api/LoggingUtil.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/api/Traits.h"
#include "fboss/agent/hw/sai/store/LoggingUtil.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/hw/sai/store/Traits.h"
#include "fboss/lib/RefMap.h"

#include <folly/dynamic.h>

#include <memory>
#include <optional>
#include <sstream>
#include <type_traits>

extern "C" {
#include <sai.h>
}

namespace facebook::fboss {

inline constexpr auto kAdapterKey2AdapterHostKey = "adapterKey2AdapterHostKey";

template <>
struct AdapterHostKeyWarmbootRecoverable<SaiNextHopGroupTraits>
    : std::false_type {};

/*
 * SaiObjectStore is the critical component of SaiStore,
 * it provides the needed operations on a single type of SaiObject
 * e.g. Port, Vlan, Route, etc... SaiStore is largely just a collection
 * of the SaiObjectStores.
 */
template <typename SaiObjectTraits>
class SaiObjectStore {
 public:
  using ObjectType = typename std::conditional<
      SaiObjectHasStats<SaiObjectTraits>::value,
      SaiObjectWithCounters<SaiObjectTraits>,
      SaiObject<SaiObjectTraits>>::type;
  using ObjectTraits = SaiObjectTraits;

  explicit SaiObjectStore(sai_object_id_t switchId) : switchId_(switchId) {}
  SaiObjectStore() {}
  ~SaiObjectStore() {
    for (auto iter : warmBootHandles_) {
      iter.second->release();
    }
  }

  void setSwitchId(sai_object_id_t switchId) {
    switchId_ = switchId;
  }

  static folly::StringPiece objectTypeName() {
    return saiObjectTypeToString(SaiObjectTraits::ObjectType);
  }
  /*
   * This routine will help load sai objects owned by the SAI Adapter.
   * For instance, sai queue objects are owned by the adapter and will not be
   * loaded during the initial reload. When a port is created, the queues will
   * be created by the SDK and the adapter keys for the queue can be retrieved.
   * Using the adapter key, the sai store can be populated with its attributes.
   */
  std::shared_ptr<ObjectType> loadObjectOwnedByAdapter(
      const typename SaiObjectTraits::AdapterKey& adapterKey) {
    static_assert(
        IsSaiObjectOwnedByAdapter<SaiObjectTraits>::value,
        "Only adapter owned SAI objects can be loaded");
    ObjectType obj(adapterKey);
    auto adapterHostKey = obj.adapterHostKey();
    auto ins = objects_.refOrEmplace(adapterHostKey, std::move(obj));
    return ins.first;
  }

  void reload(
      const folly::dynamic* adapterKeysJson,
      const folly::dynamic* adapterKeys2AdapterHostKey) {
    if (!switchId_) {
      XLOG(FATAL)
          << "Attempted to reload() on a SaiObjectStore without a switchId";
    }
    auto keys = getAdapterKeys(adapterKeysJson);
    if constexpr (SaiObjectHasConditionalAttributes<SaiObjectTraits>::value) {
      keys.erase(
          std::remove_if(
              keys.begin(),
              keys.end(),
              [](auto key) {
                auto conditionAttributes =
                    SaiApiTable::getInstance()
                        ->getApi<typename SaiObjectTraits::SaiApiT>()
                        .getAttribute(
                            key,
                            typename SaiObjectTraits::ConditionAttributes{});
                return conditionAttributes !=
                    SaiObjectTraits::kConditionAttributes;
              }),
          keys.end());
    }
    for (const auto k : keys) {
      ObjectType obj = getObject(k, adapterKeys2AdapterHostKey);
      auto adapterHostKey = obj.adapterHostKey();
      XLOGF(DBG5, "SaiStore reloaded {}", obj);
      auto ins = objects_.refOrEmplace(adapterHostKey, std::move(obj));
      if (!ins.second) {
        XLOG(FATAL) << "[" << saiObjectTypeToString(SaiObjectTraits::ObjectType)
                    << "]"
                    << " Unexpected duplicate adapterHostKey";
      }
      warmBootHandles_.emplace(adapterHostKey, ins.first);
    }
  }

  std::shared_ptr<ObjectType> setObject(
      const typename SaiObjectTraits::AdapterHostKey& adapterHostKey,
      const typename SaiObjectTraits::CreateAttributes& attributes,
      bool notify = true) {
    if constexpr (IsObjectPublisher<SaiObjectTraits>::value) {
      static_assert(
          !IsPublisherKeyCustomType<SaiObjectTraits>::value,
          "method not available for objects with publisher attributes of custom types");
    }
    XLOGF(DBG5, "SaiStore setting object {}", adapterHostKey, attributes);
    auto [object, programmed] = program(adapterHostKey, attributes);
    if (notify && programmed) {
      if constexpr (IsObjectPublisher<SaiObjectTraits>::value) {
        object->notifyAfterCreate(object);
      }
    }
    XLOGF(DBG5, "SaiStore set object {}", *object);
    return object;
  }

  std::shared_ptr<ObjectType> setObject(
      const typename SaiObjectTraits::AdapterHostKey& adapterHostKey,
      const typename SaiObjectTraits::CreateAttributes& attributes,
      const typename PublisherKey<SaiObjectTraits>::custom_type& publisherKey,
      bool notify = true) {
    static_assert(
        IsPublisherKeyCustomType<SaiObjectTraits>::value,
        "method available only for objects with publisher attributes of custom types");
    XLOGF(DBG5, "SaiStore setting object {}", adapterHostKey, attributes);
    auto [object, programmed] = program(adapterHostKey, attributes);
    if (notify && programmed) {
      if constexpr (IsObjectPublisher<SaiObjectTraits>::value) {
        object->setCustomPublisherKey(publisherKey);
        object->notifyAfterCreate(object);
      }
    }
    XLOGF(DBG5, "SaiStore set object {}", *object);
    return object;
  }

  std::shared_ptr<ObjectType> get(
      const typename SaiObjectTraits::AdapterHostKey& adapterHostKey) {
    XLOGF(DBG5, "SaiStore get object {}", adapterHostKey);
    return objects_.ref(adapterHostKey);
  }

  void release() {
    objects_.clear();
  }

  folly::dynamic adapterKeysFollyDynamic() const {
    folly::dynamic adapterKeys = folly::dynamic::array;
    for (const auto& hostKeyAndObj : objects_) {
      adapterKeys.push_back(toFollyDynamic<SaiObjectTraits>(
          hostKeyAndObj.second.lock()->adapterKey()));
    }
    return adapterKeys;
  }
  static std::vector<typename SaiObjectTraits::AdapterKey>
  adapterKeysFromFollyDynamic(const folly::dynamic& json) {
    std::vector<typename SaiObjectTraits::AdapterKey> adapterKeys;
    for (const auto& obj : json) {
      adapterKeys.push_back(fromFollyDynamic<SaiObjectTraits>(obj));
    }
    return adapterKeys;
  }
  void exitForWarmBoot() {
    for (auto itr : objects_) {
      if (auto object = itr.second.lock()) {
        object->release();
      }
    }
    objects_.clear();
  }

  const UnorderedRefMap<typename SaiObjectTraits::AdapterHostKey, ObjectType>&
  objects() const {
    return objects_;
  }

  uint64_t size() const {
    return objects_.size();
  }
  typename UnorderedRefMap<
      typename SaiObjectTraits::AdapterHostKey,
      ObjectType>::MapType::const_iterator
  begin() const {
    return objects_.begin();
  }

  typename UnorderedRefMap<
      typename SaiObjectTraits::AdapterHostKey,
      ObjectType>::MapType::const_iterator
  end() const {
    return objects_.end();
  }

 private:
  ObjectType getObject(
      typename SaiObjectTraits::AdapterKey key,
      const folly::dynamic* adapterKey2AdapterHostKey) {
    if constexpr (!AdapterHostKeyWarmbootRecoverable<SaiObjectTraits>::value) {
      if (auto ahk = getAdapterHostKey(key, adapterKey2AdapterHostKey)) {
        return ObjectType(key, ahk.value());
      }
      // API tests program using API and reload without json
      // such cases has null adapterKeys2AdapterHostKey json
    }
    return ObjectType(key);
  }

  std::pair<std::shared_ptr<ObjectType>, bool> program(
      const typename SaiObjectTraits::AdapterHostKey& adapterHostKey,
      const typename SaiObjectTraits::CreateAttributes& attributes) {
    auto ins = objects_.refOrEmplace(
        adapterHostKey, adapterHostKey, attributes, switchId_.value());
    if (!ins.second) {
      ins.first->setAttributes(attributes);
    }
    auto notify = ins.second;
    auto iter = warmBootHandles_.find(adapterHostKey);
    if (iter != warmBootHandles_.end()) {
      warmBootHandles_.erase(iter);
      notify = true;
    }
    return std::make_pair(ins.first, notify);
  }

  std::vector<typename SaiObjectTraits::AdapterKey> getAdapterKeys(
      const folly::dynamic* adapterKeysJson) const {
    return adapterKeysJson ? adapterKeysFromFollyDynamic(*adapterKeysJson)
                           : getObjectKeys<SaiObjectTraits>(switchId_.value());
  }

  std::optional<typename SaiObjectTraits::AdapterHostKey> getAdapterHostKey(
      const typename SaiObjectTraits::AdapterKey& key,
      const folly::dynamic* adapterKeys2AdapterHostKey) {
    if (!adapterKeys2AdapterHostKey) {
      return std::nullopt;
    }
    auto iter = adapterKeys2AdapterHostKey->find(folly::to<std::string>(key));
    CHECK(iter != adapterKeys2AdapterHostKey->items().end());

    return SaiObject<SaiObjectTraits>::follyDynamicToAdapterHostKey(
        iter->second);
  }

  std::optional<sai_object_id_t> switchId_;
  UnorderedRefMap<typename SaiObjectTraits::AdapterHostKey, ObjectType>
      objects_;
  std::unordered_map<
      typename SaiObjectTraits::AdapterHostKey,
      std::shared_ptr<ObjectType>>
      warmBootHandles_;
};

/*
 * SaiStore represents FBOSS's knowledge of objects and their attributes
 * that have been programmed via SAI.
 *
 */
class SaiStore {
 public:
  // Static function for getting the SaiStore folly::Singleton
  static std::shared_ptr<SaiStore> getInstance();

  SaiStore();
  explicit SaiStore(sai_object_id_t switchId);

  /*
   * Set the switch id on all the SaiObjectStores
   * Useful for the singleton mode of operation, which is constructed
   * with the default constructor, then after the switch_id is ready, that
   * is set on the SaiStore
   */
  void setSwitchId(sai_object_id_t switchId);

  /*
   * Reload the SaiStore from the current SAI state via SAI api calls.
   */
  void reload(
      const folly::dynamic* adapterKeys = nullptr,
      const folly::dynamic* adapterKeys2AdapterHostKey = nullptr);

  /*
   *
   */
  void release();

  template <typename SaiObjectTraits>
  SaiObjectStore<SaiObjectTraits>& get() {
    return std::get<SaiObjectStore<SaiObjectTraits>>(stores_);
  }
  template <typename SaiObjectTraits>
  const SaiObjectStore<SaiObjectTraits>& get() const {
    return std::get<SaiObjectStore<SaiObjectTraits>>(stores_);
  }

  std::string storeStr(sai_object_type_t objType) const;
  folly::dynamic adapterKeysFollyDynamic() const;

  void exitForWarmBoot();

  folly::dynamic adapterKeys2AdapterHostKeysFollyDynamic() const;

 private:
  sai_object_id_t switchId_{};
  std::tuple<
      SaiObjectStore<SaiAclTableGroupTraits>,
      SaiObjectStore<SaiAclTableGroupMemberTraits>,
      SaiObjectStore<SaiAclTableTraits>,
      SaiObjectStore<SaiAclEntryTraits>,
      SaiObjectStore<SaiAclCounterTraits>,
      SaiObjectStore<SaiBridgeTraits>,
      SaiObjectStore<SaiBridgePortTraits>,
      SaiObjectStore<SaiBufferPoolTraits>,
      SaiObjectStore<SaiBufferProfileTraits>,
      SaiObjectStore<SaiDebugCounterTraits>,
      SaiObjectStore<SaiPortTraits>,
      SaiObjectStore<SaiVlanTraits>,
      SaiObjectStore<SaiVlanMemberTraits>,
      SaiObjectStore<SaiRouteTraits>,
      SaiObjectStore<SaiRouterInterfaceTraits>,
      SaiObjectStore<SaiNeighborTraits>,
      SaiObjectStore<SaiFdbTraits>,
      SaiObjectStore<SaiVirtualRouterTraits>,
      SaiObjectStore<SaiIpNextHopTraits>,
      SaiObjectStore<SaiMplsNextHopTraits>,
      SaiObjectStore<SaiNextHopGroupTraits>,
      SaiObjectStore<SaiNextHopGroupMemberTraits>,
      SaiObjectStore<SaiHostifTrapGroupTraits>,
      SaiObjectStore<SaiHostifTrapTraits>,
      SaiObjectStore<SaiQueueTraits>,
      SaiObjectStore<SaiSchedulerTraits>,
      SaiObjectStore<SaiHashTraits>,
      SaiObjectStore<SaiInSegTraits>,
      SaiObjectStore<SaiQosMapTraits>,
      SaiObjectStore<SaiPortSerdesTraits>,
      SaiObjectStore<SaiWredTraits>,
      SaiObjectStore<SaiTamReportTraits>,
      SaiObjectStore<SaiTamEventActionTraits>,
      SaiObjectStore<SaiTamEventTraits>,
      SaiObjectStore<SaiTamTraits>>
      stores_;
};

template <typename SaiObjectTraits>
std::vector<typename SaiObjectTraits::AdapterKey>
keysForSaiObjStoreFromStoreJson(const folly::dynamic& json) {
  return SaiObjectStore<SaiObjectTraits>::adapterKeysFromFollyDynamic(
      json[saiObjectTypeToString(SaiObjectTraits::ObjectType)]);
}
} // namespace facebook::fboss
namespace fmt {

template <typename SaiObjectTraits>
struct formatter<facebook::fboss::SaiObjectStore<SaiObjectTraits>> {
  template <typename ParseContext>
  constexpr auto parse(ParseContext& ctx) {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(
      const facebook::fboss::SaiObjectStore<SaiObjectTraits>& store,
      FormatContext& ctx) {
    std::stringstream ss;
    ss << "Object type: "
       << facebook::fboss::saiObjectTypeToString(SaiObjectTraits::ObjectType)
       << std::endl;
    const auto& objs = store.objects();
    for (const auto& [key, object] : objs) {
      ss << fmt::format("{}", *object.lock()) << std::endl;
    }
    return format_to(ctx.out(), "{}", ss.str());
  }
};

template <typename T, typename Char>
struct is_range<facebook::fboss::SaiObjectStore<T>, Char> : std::false_type {};
}
