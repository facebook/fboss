// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSrv6SidListManager.h"

#include "fboss/agent/hw/sai/api/AddressUtil.h"
#include "fboss/agent/hw/sai/store/SaiObjectEventPublisher.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"

#include <folly/Conv.h>
#include <folly/IPAddressV6.h>
#include <folly/logging/xlog.h>

#include <sstream>
#include <string>

namespace {

// Logs AdapterHostKey fields in the same shape as warm-boot checkpoint JSON
// (SaiObject<SaiSrv6SidListTraits>::adapterHostKeyToFollyDynamic) for
// side-by-side comparison when debugging unclaimed srv6-sidlist handles.
std::string formatSrv6SidListAdapterHostKeyForWbLog(
    const facebook::fboss::SaiSrv6SidListTraits::AdapterHostKey& k) {
  using facebook::fboss::RouterInterfaceSaiId;
  using facebook::fboss::SaiSrv6SidListTraits;
  std::ostringstream oss;
  oss << "type=" << std::get<SaiSrv6SidListTraits::Attributes::Type>(k).value();
  const auto& segmentListOpt =
      std::get<std::optional<SaiSrv6SidListTraits::Attributes::SegmentList>>(k);
  if (segmentListOpt.has_value()) {
    oss << " segmentList=[";
    bool first = true;
    for (const auto& seg : segmentListOpt.value().value()) {
      if (!first) {
        oss << ",";
      }
      first = false;
      oss << folly::IPAddressV6::fromBinary(folly::ByteRange(seg.data(), 16))
                 .str();
    }
    oss << "]";
  } else {
    oss << " segmentList=(none)";
  }
  oss << " routerInterfaceId="
      << folly::to<std::string>(std::get<RouterInterfaceSaiId>(k));
  oss << " ip=" << std::get<folly::IPAddress>(k).str();
  return oss.str();
}

} // namespace

namespace facebook::fboss {

SaiSrv6SidListManager::SaiSrv6SidListManager(
    SaiStore* saiStore,
    SaiManagerTable* /*managerTable*/,
    SaiPlatform* /*platform*/)
    : saiStore_(saiStore) {}

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

ManagedSrv6SidList::ManagedSrv6SidList(
    SaiSrv6SidListManager* manager,
    SaiStore* saiStore,
    typename SaiIpNextHopTraits::AdapterHostKey nexthopKey,
    SaiSrv6SidListTraits::AdapterHostKey sidListKey,
    SaiSrv6SidListTraits::CreateAttributes attrs)
    : Base(std::move(nexthopKey)),
      manager_(manager),
      sidListKey_(std::move(sidListKey)),
      attrs_(std::move(attrs)),
      sidList_(saiStore->get<SaiSrv6SidListTraits>().setObject(
          sidListKey_,
          attrs_)) {
  XLOG(DBG2) << "[Srv6SidList WB] setObject sidListOid="
             << folly::to<std::string>(sidList_->adapterKey()) << " inputKey="
             << formatSrv6SidListAdapterHostKeyForWbLog(sidListKey_)
             << " storedKey="
             << formatSrv6SidListAdapterHostKeyForWbLog(
                    sidList_->adapterHostKey());
}

std::shared_ptr<SaiSrv6SidListHandle>
SaiSrv6SidListManager::addOrReuseSrv6SidList(
    const SaiSrv6SidListTraits::AdapterHostKey& adapterHostKey,
    const SaiSrv6SidListTraits::CreateAttributes& createAttributes,
    SaiIpNextHopTraits::AdapterHostKey subscriptionNexthopKey) {
  auto [handle, emplaced] = srv6SidLists_.refOrEmplace(adapterHostKey);
  if (emplaced) {
    handle->managedSidList = std::make_shared<ManagedSrv6SidList>(
        this,
        saiStore_,
        std::move(subscriptionNexthopKey),
        adapterHostKey,
        createAttributes);
    SaiObjectEventPublisher::getInstance()->get<SaiIpNextHopTraits>().subscribe(
        handle->managedSidList);
  }
  return handle;
}

const SaiSrv6SidListHandle* SaiSrv6SidListManager::getSrv6SidListHandle(
    const SaiSrv6SidListTraits::AdapterHostKey& adapterHostKey) const {
  return srv6SidLists_.get(adapterHostKey);
}

Srv6SidListKeyAndAttrs makeSrv6SidListKeyAndAttributes(
    RouterInterfaceSaiId rifId,
    const ResolvedNextHop& swNextHop) {
  SaiIpNextHopTraits::AdapterHostKey subscriptionNexthopKey{
      rifId, swNextHop.addr()};
  // Key SID lists by (type, segmentList, underlay RIF, underlay IP).
  // Tunnel-based SRv6 next hops use canonical (0, 0.0.0.0) in
  // SaiSrv6SidlistNextHopTraits::AdapterHostKey; SID list objects are separate.
  //
  // Build type + segment list as explicit SaiAttribute values. Relying on
  // implicit conversion from raw int / std::vector into the tuple led to
  // warm-boot checkpoints where tuple slots mis-bound.
  using Attr = SaiSrv6SidListTraits::Attributes;
  const std::vector<folly::IPAddressV6> segments = swNextHop.srv6SegmentList();
  const auto segmentBytes = toSaiIp6List(segments);
  Attr::Type sidListType{SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED};
  Attr::SegmentList segListForKey{segmentBytes};
  Attr::SegmentList segListForCreate{segmentBytes};
  SaiSrv6SidListTraits::AdapterHostKey adapterHostKey{
      sidListType,
      std::optional<Attr::SegmentList>{std::move(segListForKey)},
      rifId,
      swNextHop.addr()};
  SaiSrv6SidListTraits::CreateAttributes attrs{
      sidListType,
      std::optional<Attr::SegmentList>{std::move(segListForCreate)},
      std::nullopt};
  return {
      std::move(adapterHostKey),
      std::move(attrs),
      std::move(subscriptionNexthopKey)};
}

#endif

} // namespace facebook::fboss
