// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSrv6TunnelManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/state/Srv6Tunnel.h"

namespace facebook::fboss {

SaiSrv6TunnelManager::SaiSrv6TunnelManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* /*platform*/)
    : saiStore_(saiStore), managerTable_(managerTable) {}

SaiSrv6TunnelManager::~SaiSrv6TunnelManager() {}

void SaiSrv6TunnelManager::addSrv6Tunnel(
    const std::shared_ptr<Srv6Tunnel>& srv6Tunnel) {
  auto existTunnel = getSrv6TunnelHandle(srv6Tunnel->getID());
  if (existTunnel) {
    throw FbossError(
        "Attempted to add srv6 tunnel which already exists: ",
        srv6Tunnel->getID());
  }
  auto handle = std::make_unique<SaiSrv6TunnelHandle>();
  handles_[srv6Tunnel->getID()] = std::move(handle);
}

void SaiSrv6TunnelManager::removeSrv6Tunnel(
    const std::shared_ptr<Srv6Tunnel>& srv6Tunnel) {
  auto swId = srv6Tunnel->getID();
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Failed to remove non-existent srv6 tunnel: ", swId);
  }
  handles_.erase(itr);
}

void SaiSrv6TunnelManager::changeSrv6Tunnel(
    const std::shared_ptr<Srv6Tunnel>& oldTunnel,
    const std::shared_ptr<Srv6Tunnel>& newTunnel) {
  if (oldTunnel != newTunnel) {
    removeSrv6Tunnel(oldTunnel);
    addSrv6Tunnel(newTunnel);
  }
}

SaiSrv6TunnelHandle* FOLLY_NULLABLE
SaiSrv6TunnelManager::getSrv6TunnelHandleImpl(const std::string& swId) const {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiSrv6TunnelHandle for " << swId;
  }
  return itr->second.get();
}

} // namespace facebook::fboss
