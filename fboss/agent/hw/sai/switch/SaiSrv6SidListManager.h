// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/hw/sai/api/SaiVersion.h"

#include "fboss/agent/hw/sai/api/NextHopApi.h"
#include "fboss/agent/hw/sai/api/RouterInterfaceApi.h"
#include "fboss/agent/hw/sai/api/Srv6Api.h"
#include "fboss/agent/hw/sai/store/SaiObject.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber-defs.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventSubscriber.h"
#include "fboss/agent/state/RouteNextHop.h"
#include "fboss/lib/RefMap.h"

namespace facebook::fboss {

class SaiManagerTable;
class SaiPlatform;
class SaiStore;
class SaiSrv6SidListManager;

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
      SaiSrv6SidListManager* manager,
      SaiStore* saiStore,
      typename SaiIpNextHopTraits::AdapterHostKey nexthopKey,
      SaiSrv6SidListTraits::AdapterHostKey sidListKey,
      SaiSrv6SidListTraits::CreateAttributes attrs);

  template <typename PublishedObjectTrait>
  void afterCreateNotifyAggregateSubscriber() {
    auto publisherObject = this->getPublisherObject().lock();
    if (publisherObject) {
      SaiSrv6SidListTraits::Attributes::NextHopId nextHopIdAttr{
          publisherObject->adapterKey()};
      sidList_->setOptionalAttribute(std::move(nextHopIdAttr));
    }
  }

  template <typename PublishedObjectTrait>
  void beforeRemoveNotifyAggregateSubscriber() {
    SaiSrv6SidListTraits::Attributes::NextHopId nextHopIdAttr{
        SAI_NULL_OBJECT_ID};
    sidList_->setOptionalAttribute(std::move(nextHopIdAttr));
  }

  template <typename PublishedObjectTrait>
  void notifyLinkDownAggregateSubscriber() {}

  const SaiSrv6SidListTraits::AdapterHostKey& getSidListKey() const {
    return sidListKey_;
  }

  std::shared_ptr<SaiSrv6SidList> getSidList() const {
    return sidList_;
  }

 private:
  SaiSrv6SidListManager* manager_;
  SaiSrv6SidListTraits::AdapterHostKey sidListKey_;
  SaiSrv6SidListTraits::CreateAttributes attrs_;
  std::shared_ptr<SaiSrv6SidList> sidList_;
};
#endif

struct SaiSrv6SidListHandle {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  std::shared_ptr<ManagedSrv6SidList> managedSidList;
#endif
};

class SaiSrv6SidListManager {
 public:
  SaiSrv6SidListManager(
      SaiStore* saiStore,
      SaiManagerTable* managerTable,
      SaiPlatform* platform);

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  std::shared_ptr<SaiSrv6SidListHandle> addOrReuseSrv6SidList(
      const SaiSrv6SidListTraits::AdapterHostKey& adapterHostKey,
      const SaiSrv6SidListTraits::CreateAttributes& createAttributes,
      SaiIpNextHopTraits::AdapterHostKey subscriptionNexthopKey);

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

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
struct Srv6SidListKeyAndAttrs {
  SaiSrv6SidListTraits::AdapterHostKey adapterHostKey;
  SaiSrv6SidListTraits::CreateAttributes createAttributes;
  SaiIpNextHopTraits::AdapterHostKey subscriptionNexthopKey;
};

Srv6SidListKeyAndAttrs makeSrv6SidListKeyAndAttributes(
    RouterInterfaceSaiId rifId,
    const ResolvedNextHop& swNextHop);
#endif

} // namespace facebook::fboss
