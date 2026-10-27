// Copyright (c) Meta Platforms, Inc. and affiliates.

#include "fboss/agent/hw/sai/switch/SaiVxlanTunnelManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiTunnelUtils.h"
#include "fboss/agent/state/VxlanTunnel.h"

namespace facebook::fboss {

SaiVxlanTunnelManager::SaiVxlanTunnelManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* /*platform*/)
    : saiStore_(saiStore), managerTable_(managerTable) {}

SaiVxlanTunnelManager::~SaiVxlanTunnelManager() {}

void SaiVxlanTunnelManager::addVxlanTunnel(
    const std::shared_ptr<VxlanTunnel>& vxlanTunnel) {
  if (vxlanTunnel->getType() != TunnelType::VXLAN) {
    throw FbossError(
        "Attempted to add non-VXLAN tunnel to vxlan tunnel manager: ",
        vxlanTunnel->getID());
  }
  if (getVxlanTunnelHandle(vxlanTunnel->getID())) {
    throw FbossError(
        "Attempted to add vxlan tunnel which already exists: ",
        vxlanTunnel->getID());
  }
  SaiRouterInterfaceHandle* intfHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
          vxlanTunnel->getUnderlayIntfId());
  if (!intfHandle) {
    throw FbossError(
        "Failed to create VXLAN tunnel. "
        "No SaiRouterInterface for InterfaceID: ",
        vxlanTunnel->getUnderlayIntfId());
  }
  RouterInterfaceSaiId saiIntfId{intfHandle->adapterKey()};
  auto srcIp = vxlanTunnel->getSrcIP();
  if (!srcIp) {
    throw FbossError(
        "Failed to create VXLAN tunnel. "
        "No source IP for tunnel: ",
        vxlanTunnel->getID());
  }
  std::optional<sai_tunnel_ttl_mode_t> ttlMode;
  if (auto mode = vxlanTunnel->getTTLMode()) {
    ttlMode = getSaiTtlMode(*mode);
  }
  std::optional<sai_tunnel_dscp_mode_t> dscpMode;
  if (auto mode = vxlanTunnel->getDscpMode()) {
    dscpMode = getSaiDscpMode(*mode);
  }
  std::optional<sai_tunnel_encap_ecn_mode_t> encapEcnMode;
  std::optional<sai_tunnel_decap_ecn_mode_t> decapEcnMode;
  if (auto mode = vxlanTunnel->getEcnMode()) {
    encapEcnMode = getSaiEncapEcnMode(*mode);
    decapEcnMode = getSaiDecapEcnMode(*mode);
  }
  using Attrs = SaiVxlanTunnelTraits::Attributes;
  SaiVxlanTunnelTraits::CreateAttributes attrs{
      SAI_TUNNEL_TYPE_VXLAN,
      Attrs::UnderlayInterface(saiIntfId),
      std::optional<Attrs::EncapSrcIp>(srcIp.value()),
      ttlMode,
      dscpMode,
      encapEcnMode,
      std::nullopt, // EncapMappers (programmed separately)
      ttlMode,
      dscpMode,
      decapEcnMode,
      std::nullopt}; // DecapMappers (programmed separately)
  auto& tunnelStore = saiStore_->get<SaiVxlanTunnelTraits>();
  auto tunnelObj = tunnelStore.setObject(attrs, attrs);
  auto handle = std::make_unique<SaiVxlanTunnelHandle>();
  handle->tunnel = tunnelObj;
  handles_[vxlanTunnel->getID()] = std::move(handle);
}

void SaiVxlanTunnelManager::removeVxlanTunnel(
    const std::shared_ptr<VxlanTunnel>& vxlanTunnel) {
  auto swId = vxlanTunnel->getID();
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Failed to remove non-existent vxlan tunnel: ", swId);
  }
  handles_.erase(itr);
}

void SaiVxlanTunnelManager::changeVxlanTunnel(
    const std::shared_ptr<VxlanTunnel>& oldTunnel,
    const std::shared_ptr<VxlanTunnel>& newTunnel) {
  if (oldTunnel != newTunnel) {
    removeVxlanTunnel(oldTunnel);
    addVxlanTunnel(newTunnel);
  }
}

SaiVxlanTunnelHandle* FOLLY_NULLABLE
SaiVxlanTunnelManager::getVxlanTunnelHandleImpl(const std::string& swId) const {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiVxlanTunnelHandle for " << swId;
  }
  return itr->second.get();
}

} // namespace facebook::fboss
