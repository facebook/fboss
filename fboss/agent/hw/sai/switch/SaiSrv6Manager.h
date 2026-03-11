// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber-defs.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"
#include "fboss/lib/RefMap.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;
class SaiSrv6Manager;

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
using SaiSrv6SidList = SaiObject<SaiSrv6SidListTraits>;

class ManagedSrv6SidList : public detail::SaiObjectEventSingleSubscriber<
                               ManagedSrv6SidList,
                               SaiIpNextHopTraits> {
 public:
  using Base = detail::
      SaiObjectEventSingleSubscriber<ManagedSrv6SidList, SaiIpNextHopTraits>;
  using IpNextHopWeakPtr = std::weak_ptr<const SaiObject<SaiIpNextHopTraits>>;

  ManagedSrv6SidList(
      SaiSrv6Manager* manager,
      typename SaiIpNextHopTraits::AdapterHostKey nexthopKey,
      SaiSrv6SidListTraits::AdapterHostKey sidListKey,
      SaiSrv6SidListTraits::CreateAttributes attrs)
      : Base(std::move(nexthopKey)),
        manager_(manager),
        sidListKey_(std::move(sidListKey)),
        attrs_(std::move(attrs)) {}

  template <typename PublishedObjectTrait>
  void afterCreateNotifyAggregateSubscriber() {}

  template <typename PublishedObjectTrait>
  void beforeRemoveNotifyAggregateSubscriber() {}

  template <typename PublishedObjectTrait>
  void notifyLinkDownAggregateSubscriber() {}

  const SaiSrv6SidListTraits::AdapterHostKey& getSidListKey() const {
    return sidListKey_;
  }

 private:
  SaiSrv6Manager* manager_;
  SaiSrv6SidListTraits::AdapterHostKey sidListKey_;
  SaiSrv6SidListTraits::CreateAttributes attrs_;
};
#endif

struct SaiSrv6SidListHandle {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  std::shared_ptr<SaiSrv6SidList> sidList;
#endif
};

class SaiSrv6Manager {
 public:
  SaiSrv6Manager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  std::shared_ptr<SaiSrv6SidListHandle> addOrReuseSrv6SidList(
      const SaiSrv6SidListTraits::AdapterHostKey& adapterHostKey,
      const SaiSrv6SidListTraits::CreateAttributes& createAttributes);

  std::shared_ptr<SaiSrv6SidListHandle> insertSrv6SidList(
      std::shared_ptr<SaiSrv6SidList> sidList);

  const SaiSrv6SidListHandle* getSrv6SidListHandle(
      const SaiSrv6SidListTraits::AdapterHostKey& adapterHostKey) const;
#endif

 private:
  SaiStore* saiStore_;
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  UnorderedRefMap<SaiSrv6SidListTraits::AdapterHostKey, SaiSrv6SidListHandle>
      srv6SidLists_;
#endif
};

} // namespace facebook::fboss
