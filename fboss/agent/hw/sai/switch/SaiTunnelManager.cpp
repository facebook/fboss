// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiTunnelManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/TunnelApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/types.h"

namespace facebook::fboss {

SaiTunnelManager::SaiTunnelManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* /*platform*/)
    : saiStore_(saiStore), managerTable_(managerTable) {}

SaiTunnelManager::~SaiTunnelManager() {}

TunnelSaiId SaiTunnelManager::addTunnel(
    const std::shared_ptr<IpTunnel>& swTunnel) {
  auto& tunnelStore = saiStore_->get<SaiTunnelTraits>();
  // TTL and DSCP mode options: UNIFORM and PIPE
  // ECN has three modes instead of 2, with a customized one
  // The three values of TTL, DSCP and decap ECN will be the same value so
  // the three getters return same variable
  // For overlay interface id, we use the same value as underlay for IpinIP
  // tunnel usecase
  SaiTunnelTraits::CreateAttributes k1{
      getSaiTunnelType(swTunnel->getType()),
      swTunnel->getUnderlayIntfId(), // Underlay interface
      swTunnel->getUnderlayIntfId(), // overlay interface
      swTunnel->getTTLMode(),
      swTunnel->getDscpMode(),
      swTunnel->getEcnMode()};

  std::shared_ptr<SaiTunnel> tunnelObj = tunnelStore.setObject(k1, k1);

  auto& tunnelTermStore = saiStore_->get<SaiTunnelTermTraits>();
  SaiVirtualRouterHandle* virtualRouterHandle =
      managerTable_->virtualRouterManager().getVirtualRouterHandle(RouterID(0));
  VirtualRouterSaiId saiVirtualRouterId{
      virtualRouterHandle->virtualRouter->adapterKey()};
  SaiTunnelTermTraits::CreateAttributes k2{
      getSaiTunnelTermType(swTunnel->getTunnelTermType()),
      saiVirtualRouterId,
      swTunnel->getDstIP(), // Term Dest Ip
      swTunnel->getSrcIP(), // Term source ip
      getSaiTunnelType(swTunnel->getType()), // Tunnel Type
      tunnelObj
          ->adapterKey(), // SAI id of the tunnel, not the IpTunnel id in state
      swTunnel->getDstIPMask(),
      swTunnel->getSrcIPMask()};
  std::shared_ptr<SaiTunnelTerm> tunnelTermObj =
      tunnelTermStore.setObject(k2, k2);
  auto tunnelHandle = std::make_unique<SaiTunnelHandle>();
  tunnelHandle->tunnel = tunnelObj;
  tunnelHandle->tunnelTerm = tunnelTermObj;
  handles_[swTunnel->getID()] = std::move(tunnelHandle);
  return tunnelObj->adapterKey();
}
void SaiTunnelManager::removeTunnel(const std::shared_ptr<IpTunnel>& swTunnel) {
  // remove term, remove tunnel
  auto swId = swTunnel->getID();
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Failed to remove non-existent tunnel: ", swId);
  }
  auto handle = itr->second.get();
  handle->tunnelTerm->release();
  handle->tunnel->release();
  handles_.erase(itr);
}

void SaiTunnelManager::changeTunnel(
    const std::shared_ptr<IpTunnel>& oldTunnel,
    const std::shared_ptr<IpTunnel>& newTunnel) {
  auto swId = oldTunnel->getID();
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Failed to remove non-existent tunnel: ", swId);
  }
  // User can change any config parameter for tunnel even if they are create
  // only.
  if (oldTunnel != newTunnel) {
    removeTunnel(oldTunnel);
    addTunnel(newTunnel);
  }
}

} // namespace facebook::fboss
