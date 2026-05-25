// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiSrv6TunnelManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiTunnelUtils.h"
#include "fboss/agent/state/Srv6Tunnel.h"

namespace facebook::fboss {

#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
#endif

SaiSrv6TunnelManager::SaiSrv6TunnelManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* /*platform*/)
    : saiStore_(saiStore), managerTable_(managerTable) {}

SaiSrv6TunnelManager::~SaiSrv6TunnelManager() {}

void SaiSrv6TunnelManager::addSrv6Tunnel(
    const std::shared_ptr<Srv6Tunnel>& srv6Tunnel) {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  auto existTunnel = getSrv6TunnelHandle(srv6Tunnel->getID());
  if (existTunnel) {
    throw FbossError(
        "Attempted to add srv6 tunnel which already exists: ",
        srv6Tunnel->getID());
  }
  SaiRouterInterfaceHandle* intfHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
          srv6Tunnel->getUnderlayIntfId());
  if (!intfHandle) {
    throw FbossError(
        "Failed to create SRv6 tunnel. "
        "No SaiRouterInterface for InterfaceID: ",
        srv6Tunnel->getUnderlayIntfId());
  }
  RouterInterfaceSaiId saiIntfId{intfHandle->adapterKey()};
  auto srcIp = srv6Tunnel->getSrcIP();
  if (!srcIp) {
    throw FbossError(
        "Failed to create SRv6 tunnel. "
        "No source IP for tunnel: ",
        srv6Tunnel->getID());
  }
  auto& tunnelStore = saiStore_->get<SaiSrv6TunnelTraits>();
  std::optional<sai_tunnel_ttl_mode_t> encapTtlMode;
  if (auto ttlMode = srv6Tunnel->getTTLMode()) {
    encapTtlMode = getSaiTtlMode(*ttlMode);
  }
  std::optional<sai_tunnel_encap_ecn_mode_t> encapEcnMode;
  if (auto ecnMode = srv6Tunnel->getEcnMode()) {
    encapEcnMode = getSaiEncapEcnMode(*ecnMode);
  }
  std::optional<sai_tunnel_dscp_mode_t> encapDscpMode;
  if (auto dscpMode = srv6Tunnel->getDscpMode()) {
    encapDscpMode = getSaiDscpMode(*dscpMode);
  }
  SaiSrv6TunnelTraits::CreateAttributes attrs{
      srcIp.value(),
      SAI_TUNNEL_TYPE_SRV6,
      saiIntfId,
      encapTtlMode,
      encapEcnMode,
      encapDscpMode};
  auto tunnelObj = tunnelStore.setObject(attrs, attrs);
  auto handle = std::make_unique<SaiSrv6TunnelHandle>();
  handle->tunnel = tunnelObj;
  handles_[srv6Tunnel->getID()] = std::move(handle);
#else
  throw FbossError(
      "SRv6 tunnels require SAI API version >= 1.12.0, "
      "tunnel: ",
      srv6Tunnel->getID());
#endif
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
