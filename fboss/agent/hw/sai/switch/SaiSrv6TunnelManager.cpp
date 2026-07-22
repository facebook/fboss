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

bool SaiSrv6TunnelManager::isSrv6DecapTunnelSupported() const {
#if defined(BRCM_SAI_SDK_XGS_GTE_15_0)
  return true;
#else
  // TODO(zecheng): only Broadcom XGS SAI SDK >= 15.0 supports SRv6 decap
  // tunnel creation.
  return false;
#endif
}

void SaiSrv6TunnelManager::addSrv6Tunnel(
    const std::shared_ptr<Srv6Tunnel>& srv6Tunnel) {
  if (srv6Tunnel->getType() == TunnelType::SRV6_DECAP) {
    addSrv6DecapTunnel(srv6Tunnel);
    return;
  }
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
  using Attrs = SaiSrv6TunnelTraits::Attributes;
  SaiSrv6TunnelTraits::CreateAttributes attrs{
      SAI_TUNNEL_TYPE_SRV6,
      std::optional<Attrs::UnderlayInterface>(saiIntfId),
      std::optional<Attrs::EncapSrcIp>(srcIp.value()),
      encapTtlMode,
      encapEcnMode,
      encapDscpMode,
      std::nullopt, // DecapTtlMode (encap tunnel)
      std::nullopt, // DecapDscpMode (encap tunnel)
      std::nullopt}; // DecapEcnMode (encap tunnel)
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

void SaiSrv6TunnelManager::addSrv6DecapTunnel(
    const std::shared_ptr<Srv6Tunnel>& srv6Tunnel) {
#if SAI_API_VERSION >= SAI_VERSION(1, 12, 0)
  if (!isSrv6DecapTunnelSupported()) {
    XLOG(WARNING)
        << "SRv6 decap tunnel configured but unsupported on this SDK/ASIC; "
        << "skipping creation, decap MySid entries will have no tunnel: "
        << srv6Tunnel->getID();
    return;
  }
  if (getSrv6TunnelHandle(srv6Tunnel->getID())) {
    throw FbossError(
        "Attempted to add srv6 decap tunnel which already exists: ",
        srv6Tunnel->getID());
  }
  if (decapTunnelId_) {
    throw FbossError(
        "Only one SRv6 decap tunnel is supported; already have: ",
        *decapTunnelId_,
        ", cannot add: ",
        srv6Tunnel->getID());
  }
  std::optional<sai_tunnel_ttl_mode_t> ttlMode;
  if (auto mode = srv6Tunnel->getTTLMode()) {
    ttlMode = getSaiTtlMode(*mode);
  }
  std::optional<sai_tunnel_dscp_mode_t> dscpMode;
  if (auto mode = srv6Tunnel->getDscpMode()) {
    dscpMode = getSaiDscpMode(*mode);
  }
  std::optional<sai_tunnel_decap_ecn_mode_t> ecnMode;
  if (auto mode = srv6Tunnel->getEcnMode()) {
    ecnMode = getSaiDecapEcnMode(*mode);
  }
  // Decap tunnel: only the decap modes are set; encap fields stay unset.
  SaiSrv6TunnelTraits::CreateAttributes attrs{
      SAI_TUNNEL_TYPE_SRV6,
      std::nullopt, // UnderlayInterface (decap tunnel)
      std::nullopt, // EncapSrcIp (decap tunnel)
      std::nullopt, // EncapTtlMode (decap tunnel)
      std::nullopt, // EncapEcnMode (decap tunnel)
      std::nullopt, // EncapDscpMode (decap tunnel)
      ttlMode,
      dscpMode,
      ecnMode};
  auto& tunnelStore = saiStore_->get<SaiSrv6TunnelTraits>();
  auto tunnelObj = tunnelStore.setObject(attrs, attrs);
  auto handle = std::make_unique<SaiSrv6TunnelHandle>();
  handle->tunnel = tunnelObj;
  handles_[srv6Tunnel->getID()] = std::move(handle);
  decapTunnelId_ = srv6Tunnel->getID();
#else
  throw FbossError(
      "SRv6 decap tunnels require SAI API version >= 1.12.0, tunnel: ",
      srv6Tunnel->getID());
#endif
}

void SaiSrv6TunnelManager::removeSrv6Tunnel(
    const std::shared_ptr<Srv6Tunnel>& srv6Tunnel) {
  if (srv6Tunnel->getType() == TunnelType::SRV6_DECAP &&
      !isSrv6DecapTunnelSupported()) {
    return;
  }
  auto swId = srv6Tunnel->getID();
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Failed to remove non-existent srv6 tunnel: ", swId);
  }
  handles_.erase(itr);
  if (decapTunnelId_ == swId) {
    decapTunnelId_.reset();
  }
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

const SaiSrv6TunnelHandle* FOLLY_NULLABLE
SaiSrv6TunnelManager::getDecapTunnelHandle() const {
  if (!decapTunnelId_) {
    return nullptr;
  }
  return getSrv6TunnelHandleImpl(*decapTunnelId_);
}

} // namespace facebook::fboss
