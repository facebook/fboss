// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"

#include <memory>
#include <variant>

namespace facebook::fboss {

class MySid;
class SaiManagerTable;
class SaiPlatform;
class SaiStore;
class SaiSrv6MySidManager;
struct SaiNextHopGroupHandle;

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
using SaiMySid = SaiObject<SaiMySidEntryTraits>;

template <typename T>
class ManagedNextHop;

class ManagedMySidNextHop
    : public detail::SaiObjectEventSubscriber<SaiIpNextHopTraits> {
 public:
  using PublisherObject = std::shared_ptr<const SaiObject<SaiIpNextHopTraits>>;
  ManagedMySidNextHop(
      SaiSrv6MySidManager* manager,
      SaiMySidEntryTraits::AdapterHostKey mySidKey,
      std::shared_ptr<ManagedNextHop<SaiIpNextHopTraits>> managedNextHop);
  void afterCreate(PublisherObject nexthop) override;
  void beforeRemove() override;
  void linkDown() override {}
  sai_object_id_t adapterKey() const;
  using detail::SaiObjectEventSubscriber<SaiIpNextHopTraits>::isReady;

 private:
  SaiSrv6MySidManager* manager_;
  SaiMySidEntryTraits::AdapterHostKey mySidKey_;
  std::shared_ptr<ManagedNextHop<SaiIpNextHopTraits>> managedNextHop_;
};

SaiMySidEntryTraits::AdapterHostKey getMySidAdapterHostKey(
    const MySid& mySid,
    SaiManagerTable* managerTable);

struct SaiMySidEntryHandle {
  using NextHopHandle = std::variant<
      std::shared_ptr<SaiNextHopGroupHandle>,
      std::shared_ptr<ManagedMySidNextHop>>;
  NextHopHandle nexthopHandle;
  std::shared_ptr<SaiMySid> mySidEntry;
};
#endif

class SaiSrv6MySidManager {
 public:
  SaiSrv6MySidManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  void addMySidEntry(const std::shared_ptr<MySid>& mySid);
  void removeMySidEntry(const std::shared_ptr<MySid>& mySid);
  void changeMySidEntry(
      const std::shared_ptr<MySid>& oldMySid,
      const std::shared_ptr<MySid>& newMySid);

  std::shared_ptr<SaiObject<SaiMySidEntryTraits>> getMySidObject(
      const SaiMySidEntryTraits::AdapterHostKey& key);
#endif

 private:
  SaiStore* saiStore_;
  SaiManagerTable* managerTable_;
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  folly::F14FastMap<
      SaiMySidEntryTraits::AdapterHostKey,
      std::unique_ptr<SaiMySidEntryHandle>>
      handles_;
#endif
};

} // namespace facebook::fboss
