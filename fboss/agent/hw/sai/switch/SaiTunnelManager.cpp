// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/hw/sai/switch/SaiTunnelManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/api/TunnelApi.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiRouterInterfaceManager.h"
#include "fboss/agent/hw/sai/switch/SaiVirtualRouterManager.h"
#include "fboss/agent/state/IpTunnel.h"
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
  auto existTunnel = getTunnelHandle(swTunnel->getID());
  if (existTunnel) {
    throw FbossError(
        "Attempted to add tunnel which already exists: ",
        swTunnel->getID(),
        " SAI id: ",
        existTunnel->tunnel->adapterKey());
  }
  SaiRouterInterfaceHandle* intfHandle =
      managerTable_->routerInterfaceManager().getRouterInterfaceHandle(
          swTunnel->getUnderlayIntfId());
  if (!intfHandle) {
    throw FbossError(
        "Failed to create tunnel from IpTunnel. "
        "No SaiRouterInterface for InterfaceID: ",
        swTunnel->getUnderlayIntfId());
  }
  RouterInterfaceSaiId saiIntfId{intfHandle->adapterKey()};
  auto& tunnelStore = saiStore_->get<SaiTunnelTraits>();
  // TTL and DSCP mode options: UNIFORM and PIPE
  // ECN has three modes instead of 2, with a customized one
  // The three values of TTL, DSCP and decap ECN will be the same value so
  // the three getters return same variable
  // For overlay interface id, we use the same value as underlay for IpinIP
  // tunnel usecase
  SaiTunnelTraits::CreateAttributes k1{
      getSaiTunnelType(swTunnel->getType()),
      saiIntfId,
      saiIntfId,
      getSaiTtlMode(swTunnel->getTTLMode()),
      getSaiDscpMode(swTunnel->getDscpMode()),
      getSaiDecapEcnMode(swTunnel->getEcnMode())};

  std::shared_ptr<SaiTunnel> tunnelObj = tunnelStore.setObject(k1, k1);

  auto& tunnelTermStore = saiStore_->get<SaiTunnelTermTraits>();
  SaiVirtualRouterHandle* virtualRouterHandle =
      managerTable_->virtualRouterManager().getVirtualRouterHandle(RouterID(0));
  VirtualRouterSaiId saiVirtualRouterId{
      virtualRouterHandle->virtualRouter->adapterKey()};
  if (swTunnel->getTunnelTermType() == P2MP) {
    SaiTunnelTermTraits::CreateAttributes k2{
        getSaiTunnelTermType(swTunnel->getTunnelTermType()),
        saiVirtualRouterId,
        swTunnel->getSrcIP(), // Term Dest Ip
        getSaiTunnelType(swTunnel->getType()), // Tunnel Type
        // SAI id of the tunnel, not the IpTunnel id in state
        tunnelObj->adapterKey()};

    std::shared_ptr<SaiTunnelTerm> tunnelTermObj =
        tunnelTermStore.setObject(k2, k2);
    auto tunnelHandle = std::make_unique<SaiTunnelHandle>();
    tunnelHandle->tunnel = tunnelObj;
    tunnelHandle->tunnelTerm = tunnelTermObj;
    handles_[swTunnel->getID()] = std::move(tunnelHandle);
  } else {
    throw FbossError("Tunnel Term types other than P2MP are not supported yet");
  }
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

// private const getter for use by const and non-const getters
SaiTunnelHandle* FOLLY_NULLABLE
SaiTunnelManager::getTunnelHandleImpl(std::string swId) const {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiTunnelHandle for " << swId;
  }
  return itr->second.get();
}

sai_tunnel_type_t SaiTunnelManager::getSaiTunnelType(TunnelType type) {
  switch (type) {
    case IPINIP:
      return SAI_TUNNEL_TYPE_IPINIP;
    default:
      throw FbossError("Failed to convert IpTunnel type to SAI type: ", type);
  }
}

sai_tunnel_term_table_entry_type_t SaiTunnelManager::getSaiTunnelTermType(
    TunnelTermType type) {
  switch (type) {
    case P2MP:
      return SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP;
    case MP2MP:
      return SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_MP2MP;
    default:
      throw FbossError(
          "Failed to convert IpTunnel term type to SAI type: ", type);
  }
}
sai_tunnel_ttl_mode_t SaiTunnelManager::getSaiTtlMode(cfg::IpTunnelMode m) {
  switch (m) {
    case cfg::IpTunnelMode::UNIFORM:
      return SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL;
    case cfg::IpTunnelMode::PIPE:
      return SAI_TUNNEL_TTL_MODE_PIPE_MODEL;
    default:
      throw FbossError("Failed to convert TTL mode to SAI type: ", m);
  }
}
sai_tunnel_dscp_mode_t SaiTunnelManager::getSaiDscpMode(cfg::IpTunnelMode m) {
  switch (m) {
    case cfg::IpTunnelMode::UNIFORM:
      return SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL;
    case cfg::IpTunnelMode::PIPE:
      return SAI_TUNNEL_DSCP_MODE_PIPE_MODEL;
    default:
      throw FbossError("Failed to convert DSCP mode to SAI type: ", m);
  }
}
sai_tunnel_decap_ecn_mode_t SaiTunnelManager::getSaiDecapEcnMode(
    cfg::IpTunnelMode m) {
  switch (m) {
    case cfg::IpTunnelMode::UNIFORM:
      return SAI_TUNNEL_DECAP_ECN_MODE_STANDARD;
    case cfg::IpTunnelMode::PIPE:
      return SAI_TUNNEL_DECAP_ECN_MODE_COPY_FROM_OUTER;
    case cfg::IpTunnelMode::USER:
      return SAI_TUNNEL_DECAP_ECN_MODE_USER_DEFINED;
    default:
      throw FbossError("Failed to convert ECN mode to SAI type: ", m);
  }
}

} // namespace facebook::fboss
