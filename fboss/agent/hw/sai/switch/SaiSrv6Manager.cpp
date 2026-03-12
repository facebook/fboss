// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSrv6Manager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"

namespace facebook::fboss {

SaiSrv6Manager::SaiSrv6Manager(
    SaiStore* saiStore,
    SaiManagerTable* /*managerTable*/,
    SaiPlatform* /*platform*/)
    : saiStore_(saiStore) {}

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)

std::shared_ptr<SaiSrv6SidListHandle> SaiSrv6Manager::addOrReuseSrv6SidList(
    const SaiSrv6SidListTraits::AdapterHostKey& adapterHostKey,
    const SaiSrv6SidListTraits::CreateAttributes& createAttributes) {
  auto [handle, emplaced] = srv6SidLists_.refOrEmplace(adapterHostKey);
  if (emplaced) {
    auto& store = saiStore_->get<SaiSrv6SidListTraits>();
    handle->sidList = store.setObject(adapterHostKey, createAttributes);
  }
  return handle;
}

const SaiSrv6SidListHandle* SaiSrv6Manager::getSrv6SidListHandle(
    const SaiSrv6SidListTraits::AdapterHostKey& adapterHostKey) const {
  return srv6SidLists_.get(adapterHostKey);
}

std::pair<
    SaiSrv6SidListTraits::AdapterHostKey,
    SaiSrv6SidListTraits::CreateAttributes>
makeSrv6SidListKeyAndAttributes(
    RouterInterfaceSaiId rifId,
    const ResolvedNextHop& swNextHop) {
  SaiSrv6SidListTraits::AdapterHostKey key{
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED,
      swNextHop.srv6SegmentList(),
      rifId,
      swNextHop.addr()};
  SaiSrv6SidListTraits::CreateAttributes attrs{
      SAI_SRV6_SIDLIST_TYPE_ENCAPS_RED,
      swNextHop.srv6SegmentList(),
      std::nullopt};
  return {std::move(key), std::move(attrs)};
}

#endif

} // namespace facebook::fboss
