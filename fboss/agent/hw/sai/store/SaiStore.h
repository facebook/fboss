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

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/AclApi.h"
#include "fboss/agent/hw/sai/api/AdapterKeySerializers.h"
#include "fboss/agent/hw/sai/api/LagApi.h"
#include "fboss/agent/hw/sai/api/LoggingUtil.h"
#include "fboss/agent/hw/sai/api/NextHopGroupApi.h"
#include "fboss/agent/hw/sai/api/SaiApiTable.h"
#include "fboss/agent/hw/sai/api/SaiObjectApi.h"
#include "fboss/agent/hw/sai/api/Traits.h"
#include "fboss/agent/hw/sai/store/LoggingUtil.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiObjectWithCounters.h"
#include "fboss/agent/hw/sai/store/Traits.h"
#include "fboss/lib/RefMap.h"

#include <folly/json/dynamic.h>

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

template <>
struct AdapterHostKeyWarmbootRecoverable<SaiAclTableTraits> : std::false_type {
};

#if defined(BRCM_SAI_SDK_XGS_AND_DNX)
template <>
struct AdapterHostKeyWarmbootRecoverable<SaiWredTraits> : std::false_type {};

#endif

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

  explicit SaiObjectStore(sai_object_id_t switchId) : saiSwitchId_(switchId) {}
  SaiObjectStore() {}
  ~SaiObjectStore() {
    for (auto iter : warmBootHandles_) {
      iter.second->release();
    }
  }

  void setSwitchId(sai_object_id_t switchId) {
    saiSwitchId_ = switchId;
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
      const typename SaiObjectTraits::AdapterKey& adapterKey,
      bool addToWarmbootHandles = false) {
    auto object = reloadObject(adapterKey, addToWarmbootHandles);
    object->setOwnedByAdapter(true);
    XLOGF(DBG5, "SaiStore reload by adapter {}", *object.get());
    return object;
  }

  std::shared_ptr<ObjectType> reloadObject(
      const typename SaiObjectTraits::AdapterKey& adapterKey,
      bool addToWarmbootHandles = false) {
    ObjectType temporary(adapterKey);
    auto adapterHostKey = temporary.adapterHostKey();
    auto obj = objects_.ref(adapterHostKey);
    if (!obj) {
      auto ins =
          objects_.refOrInsert(adapterHostKey, std::move(temporary), true);
      obj = ins.first;
    } else {
      // destroy temporary without removing underlying sai object
      temporary.release();
    }
    if (addToWarmbootHandles) {
      warmBootHandles_.emplace(adapterHostKey, obj);
    } else {
      auto iter = warmBootHandles_.find(adapterHostKey);
      if (iter != warmBootHandles_.end()) {
        // Adapter keys are discovered by sai store in two ways
        // 1. by calling get object keys api which is possible in cold and warm
        // boot.
        // 2. by looking into saved hw switch state, only possible in warm boot
        //
        // For objects owned by adapter keys, first is possible ONLY if adapter
        // supports get object keys api for a given object type. Unfortunately,
        // not all adapters may support get object keys api for all objects. For
        // such adapters, warmboot handles may not have this key during cold
        // boot. For adapters which support get object keys api, warm boot
        // handles will always have this object.
        warmBootHandles_.erase(iter);
      }
    }
    return obj;
  }

  void reload(
      const folly::dynamic* adapterKeysJson,
      const folly::dynamic* adapterKeys2AdapterHostKey) {
    if (!saiSwitchId_) {
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
    for (const auto& k : keys) {
      ObjectType obj = getObject(k, adapterKeys2AdapterHostKey);
      auto adapterHostKey = obj.adapterHostKey();
      XLOGF(DBG5, "SaiStore reloaded {}", obj);
      auto ins = objects_.refOrInsert(adapterHostKey, std::move(obj));
      if (!ins.second) {
        XLOG(FATAL) << "[" << saiObjectTypeToString(SaiObjectTraits::ObjectType)
                    << "]" << " Unexpected duplicate adapterHostKey";
      }
      warmBootHandles_.emplace(adapterHostKey, ins.first);
    }
  }

  // must be invoked during warm boot only, before entry has been warm booted.
  // this is needed for updating of acl table schema, this requires removing all
  // acl entries and putting it back.
  template <typename T = SaiObjectTraits>
  void addWarmbootHandle(
      const typename T::AdapterHostKey& adapterHostKey,
      const typename T::CreateAttributes& attributes) {
    auto itr = warmBootHandles_.find(adapterHostKey);
    CHECK(itr == warmBootHandles_.end());
    auto object = std::make_shared<ObjectType>(
        ObjectType(adapterHostKey, attributes, saiSwitchId_.value()));
    warmBootHandles_.emplace(adapterHostKey, object);
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
    XLOGF(
        DBG5,
        "SaiStore setting {} object {}",
        objectTypeName(),
        adapterHostKey);
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
    XLOGF(
        DBG5,
        "SaiStore setting {} object {}",
        objectTypeName(),
        adapterHostKey);
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

  template <typename AttrT>
  void setObjects(
      std::vector<typename SaiObjectTraits::AdapterHostKey>& adapterHostKeys,
      std::vector<AttrT>& attributes,
      bool notify = true) {
    std::vector<typename SaiObjectTraits::AdapterKey> adapterKeys;
    CHECK_EQ(adapterHostKeys.size(), attributes.size());
    for (const auto& adapterHostKey : adapterHostKeys) {
      XLOGF(
          DBG5,
          "SaiStore bulk setting {} object {}",
          objectTypeName(),
          adapterHostKey);
      auto existingObj = objects_.ref(adapterHostKey);
      if (!existingObj) {
        throw FbossError("Bulk set called for non existing object");
      }
      auto adapterKey = existingObj->adapterKey();
      adapterKeys.emplace_back(adapterKey);
    }

    SaiObject<SaiObjectTraits>::bulkSetAttributes(adapterKeys, attributes);

    auto idx = 0;
    for (const auto& adapterHostKey : adapterHostKeys) {
      auto existingObj = objects_.ref(adapterHostKey);
      existingObj->checkAndSetAttribute(
          std::optional<AttrT>{std::forward<AttrT>(attributes[idx++])},
          true /* skipHwWrite */);
      if (notify) {
        if constexpr (IsObjectPublisher<SaiObjectTraits>::value) {
          existingObj->notifyAfterCreate(existingObj);
        }
      }
      XLOGF(DBG5, "SaiStore bulk set object {}", *existingObj);
    }
  }

  std::shared_ptr<ObjectType> get(
      const typename SaiObjectTraits::AdapterHostKey& adapterHostKey) {
    XLOGF(DBG5, "SaiStore get object {}", adapterHostKey);
    auto itr = warmBootHandles_.find(adapterHostKey);
    if (itr != warmBootHandles_.end()) {
      return itr->second;
    }
    return objects_.ref(adapterHostKey);
  }

  std::shared_ptr<ObjectType> getWarmbootHandle(
      const typename SaiObjectTraits::AdapterHostKey& adapterHostKey) {
    XLOGF(DBG5, "SaiStore get from WB cache object {}", adapterHostKey);
    auto itr = warmBootHandles_.find(adapterHostKey);
    if (itr != warmBootHandles_.end()) {
      return itr->second;
    }
    return nullptr;
  }

  std::shared_ptr<ObjectType> find(
      const typename SaiObjectTraits::AdapterKey& adapterKey) {
    XLOGF(DBG5, "SaiStore find object {}", adapterKey);
    for (auto iter : warmBootHandles_) {
      if (iter.second->adapterKey() == adapterKey) {
        return iter.second;
      }
    }
    for (auto iter : objects_) {
      auto obj = iter.second.lock();
      if (obj->adapterKey() == adapterKey) {
        return obj;
      }
    }
    return nullptr;
  }

  void release() {
    objects_.clear();
  }

  folly::dynamic adapterKeysFollyDynamic() const {
    folly::dynamic adapterKeys = folly::dynamic::array;
    for (const auto& hostKeyAndObj : objects_) {
      auto obj = hostKeyAndObj.second.lock();
      if (!obj->live()) {
        continue;
      }
      adapterKeys.push_back(toFollyDynamic<SaiObjectTraits>(obj->adapterKey()));
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

  bool hasUnexpectedUnclaimedWarmbootHandles(
      bool includeAdapterOwned = false) const {
    auto handlesCount = warmBootHandlesCount(includeAdapterOwned);
    bool unclaimedHandles = handlesCount > 0 &&
        (includeAdapterOwned ||
         !IsSaiObjectOwnedByAdapter<SaiObjectTraits>::value);
    XLOGF(
        DBG1,
        "unexpected warmboot handles {} entries: {}, includeAdapterOwned: {}",
        objectTypeName(),
        unclaimedHandles,
        includeAdapterOwned);
    return unclaimedHandles;
  }

  void printWarmBootHandles() const {
    if (warmBootHandlesCount()) {
      XLOGF(DBG1, "unclaimed {} entries", objectTypeName());
    }
    for (auto iter : warmBootHandles_) {
      XLOGF(DBG1, "{}", *iter.second);
    }
  }

  void removeUnclaimedWarmbootHandlesIf(
      std::function<bool(const std::shared_ptr<ObjectType>&)> condition,
      bool includeAdapterOwned = false) {
    if (!hasUnexpectedUnclaimedWarmbootHandles(includeAdapterOwned)) {
      return;
    }
    auto iter = std::begin(warmBootHandles_);
    while (iter != std::end(warmBootHandles_)) {
      if (!condition(iter->second)) {
        iter++;
        continue;
      }
      iter = warmBootHandles_.erase(iter);
    }
  }

  void removeUnexpectedUnclaimedWarmbootHandles(
      bool includeAdapterOwned = false) {
    // delete all unclaimed handles
    removeUnclaimedWarmbootHandlesIf(
        [](const auto&) { return true; }, includeAdapterOwned);
  }

 private:
  size_t warmBootHandlesCount(bool includeAdapterOwned = false) const {
    return std::count_if(
        std::begin(warmBootHandles_),
        std::end(warmBootHandles_),
        [includeAdapterOwned](const auto& handle) {
          return includeAdapterOwned || !handle.second->isOwnedByAdapter();
        });
  }

  ObjectType getObject(
      typename ObjectTraits::AdapterKey key,
      const folly::dynamic* adapterKey2AdapterHostKey) {
    if constexpr (!AdapterHostKeyWarmbootRecoverable<SaiObjectTraits>::value) {
      auto ahk = getAdapterHostKey(key, adapterKey2AdapterHostKey);
      if (ahk) {
        return ObjectType(key, ahk.value());
      } else {
        if constexpr (std::is_same_v<ObjectTraits, SaiAclTableTraits>) {
          // TODO(pshaikh): hack to allow warm boot from version which doesn't
          // save ahk
          return ObjectType(key, SaiAclTableTraits::AdapterHostKey{kAclTable1});
        }
        if constexpr (std::is_same_v<ObjectTraits, SaiNextHopGroupTraits>) {
          // a special handling has been added to deal with next hop group to
          // recover itself even if adapter host key is not saved in warm boot
          // state.
          return ObjectType(key);
        }
#if defined(BRCM_SAI_SDK_XGS)
        if constexpr (std::is_same_v<ObjectTraits, SaiWredTraits>) {
          // Allow warm boot from version which doesn't save ahk
          // TODO(zecheng): Remove after device warmbooted to 8.2
          return ObjectType(key);
        }
#endif
        throw FbossError(
            "attempting to load an object whose adapter host key is not found.");
      }
    } else {
      return ObjectType(key);
    }
  }

  std::pair<std::shared_ptr<ObjectType>, bool> program(
      const typename SaiObjectTraits::AdapterHostKey& adapterHostKey,
      const typename SaiObjectTraits::CreateAttributes& attributes) {
    auto existingObj = objects_.ref(adapterHostKey);
    auto ins = existingObj
        ? std::make_pair(existingObj, false)
        : objects_.refOrInsert(
              adapterHostKey,
              ObjectType(adapterHostKey, attributes, saiSwitchId_.value()),
              true /*force*/);
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
    return adapterKeysJson
        ? adapterKeysFromFollyDynamic(*adapterKeysJson)
        : getObjectKeys<SaiObjectTraits>(saiSwitchId_.value());
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

  std::optional<sai_object_id_t> saiSwitchId_;
  UnorderedRefMap<typename SaiObjectTraits::AdapterHostKey, ObjectType>
      objects_;
  std::unordered_map<
      typename SaiObjectTraits::AdapterHostKey,
      std::shared_ptr<ObjectType>>
      warmBootHandles_;
};

/*
 * Specialize SaiSwitchObj to allow for stand alone construction
 * since we don't create a object store for SaiSwitchObj
 */
class SaiSwitchObj : public SaiObjectWithCounters<SaiSwitchTraits> {
 public:
  template <typename... Args>
  SaiSwitchObj(Args&&... args)
      : SaiObjectWithCounters<SaiSwitchTraits>(std::forward<Args>(args)...) {}
};
/*
 * SaiStore represents FBOSS's knowledge of objects and their attributes
 * that have been programmed via SAI.
 *
 */
class SaiStore {
 public:
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

  void checkUnexpectedUnclaimedWarmbootHandles() const;

  void removeUnexpectedUnclaimedWarmbootHandles();

  void printWarmbootHandles() const;

 private:
  sai_object_id_t saiSwitchId_{};
  std::tuple<
      SaiObjectStore<SaiAclTableGroupTraits>,
      SaiObjectStore<SaiAclTableGroupMemberTraits>,
      SaiObjectStore<SaiAclTableTraits>,
      SaiObjectStore<SaiAclEntryTraits>,
      SaiObjectStore<SaiAclCounterTraits>,
#if SAI_API_VERSION >= SAI_VERSION(1, 14, 0)
      SaiObjectStore<SaiArsTraits>,
      SaiObjectStore<SaiArsProfileTraits>,
#endif
      SaiObjectStore<SaiBridgeTraits>,
      SaiObjectStore<SaiBridgePortTraits>,
      SaiObjectStore<SaiBufferPoolTraits>,
      SaiObjectStore<SaiBufferProfileTraits>,
      SaiObjectStore<SaiIngressPriorityGroupTraits>,
#if SAI_API_VERSION >= SAI_VERSION(1, 10, 0)
      SaiObjectStore<SaiCounterTraits>,
#endif
      SaiObjectStore<SaiInPortDebugCounterTraits>,
      SaiObjectStore<SaiOutPortDebugCounterTraits>,
      SaiObjectStore<SaiSystemPortTraits>,
      SaiObjectStore<SaiPortTraits>,
      SaiObjectStore<SaiUdfTraits>,
      SaiObjectStore<SaiUdfGroupTraits>,
      SaiObjectStore<SaiUdfMatchTraits>,
      SaiObjectStore<SaiVlanTraits>,
      SaiObjectStore<SaiVlanMemberTraits>,
      SaiObjectStore<SaiRouteTraits>,
      SaiObjectStore<SaiVlanRouterInterfaceTraits>,
      SaiObjectStore<SaiMplsRouterInterfaceTraits>,
      SaiObjectStore<SaiPortRouterInterfaceTraits>,
      SaiObjectStore<SaiFdbTraits>,
      SaiObjectStore<SaiVirtualRouterTraits>,
      SaiObjectStore<SaiNextHopGroupMemberTraits>,
      SaiObjectStore<SaiNextHopGroupTraits>,
      SaiObjectStore<SaiIpNextHopTraits>,
      SaiObjectStore<SaiLocalMirrorTraits>,
      SaiObjectStore<SaiEnhancedRemoteMirrorTraits>,
      SaiObjectStore<SaiSflowMirrorTraits>,
      SaiObjectStore<SaiMplsNextHopTraits>,
      SaiObjectStore<SaiNeighborTraits>,
      SaiObjectStore<SaiHostifTrapGroupTraits>,
      SaiObjectStore<SaiHostifTrapTraits>,
      SaiObjectStore<SaiHostifUserDefinedTrapTraits>,
      SaiObjectStore<SaiQueueTraits>,
      SaiObjectStore<SaiSchedulerTraits>,
      SaiObjectStore<SaiSamplePacketTraits>,
      SaiObjectStore<SaiHashTraits>,
      SaiObjectStore<SaiInSegTraits>,
      SaiObjectStore<SaiQosMapTraits>,
      SaiObjectStore<SaiPortSerdesTraits>,
      SaiObjectStore<SaiPortConnectorTraits>,
      SaiObjectStore<SaiWredTraits>,
      SaiObjectStore<SaiTamReportTraits>,
      SaiObjectStore<SaiTamEventActionTraits>,
      SaiObjectStore<SaiTamEventTraits>,
      SaiObjectStore<SaiTamTraits>,
      SaiObjectStore<SaiTunnelTraits>,
      SaiObjectStore<SaiP2MPTunnelTermTraits>,
      SaiObjectStore<SaiP2PTunnelTermTraits>,
      SaiObjectStore<SaiLagTraits>,
      SaiObjectStore<SaiLagMemberTraits>,
      SaiObjectStore<SaiMacsecTraits>,
      SaiObjectStore<SaiMacsecFlowTraits>,
      SaiObjectStore<SaiMacsecPortTraits>,
      SaiObjectStore<SaiMacsecSCTraits>,
      SaiObjectStore<SaiMacsecSATraits>>
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
  constexpr auto parse(ParseContext& ctx) const {
    return ctx.begin();
  }

  template <typename FormatContext>
  auto format(
      const facebook::fboss::SaiObjectStore<SaiObjectTraits>& store,
      FormatContext& ctx) const {
    std::stringstream ss;
    ss << "Object type: "
       << facebook::fboss::saiObjectTypeToString(SaiObjectTraits::ObjectType)
       << std::endl;
    const auto& objs = store.objects();
    for (const auto& [key, object] : objs) {
      std::ignore = key;
      ss << fmt::format("{}", *object.lock()) << std::endl;
    }
    return format_to(ctx.out(), "{}", ss.str());
  }
};

template <typename T, typename Char>
struct is_range<facebook::fboss::SaiObjectStore<T>, Char> : std::false_type {};
} // namespace fmt
