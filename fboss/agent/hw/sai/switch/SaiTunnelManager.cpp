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

namespace {

sai_tunnel_type_t getSaiTunnelType(cfg::TunnelType type) {
  switch (type) {
    case cfg::TunnelType::IP_IN_IP:
      return SAI_TUNNEL_TYPE_IPINIP;
  }
  throw FbossError("Failed to convert tunnel type to SAI type: ", type);
}

sai_tunnel_term_table_entry_type_t getSaiTunnelTermType(
    cfg::TunnelTerminationType type) {
  switch (type) {
    case cfg::TunnelTerminationType::P2MP:
      return SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2MP;
    case cfg::TunnelTerminationType::MP2MP:
      return SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_MP2MP;
    case cfg::TunnelTerminationType::P2P:
      return SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_P2P;
    case cfg::TunnelTerminationType::MP2P:
      return SAI_TUNNEL_TERM_TABLE_ENTRY_TYPE_MP2P;
  }
  throw FbossError("Failed to convert tunnel term type to SAI type: ", type);
}

sai_tunnel_ttl_mode_t getSaiTtlMode(cfg::IpTunnelMode mode) {
  switch (mode) {
    case cfg::IpTunnelMode::UNIFORM:
      return SAI_TUNNEL_TTL_MODE_UNIFORM_MODEL;
    case cfg::IpTunnelMode::PIPE:
      return SAI_TUNNEL_TTL_MODE_PIPE_MODEL;
    case cfg::IpTunnelMode::USER:
      break;
  }
  throw FbossError("Failed to convert TTL mode to SAI type: ", mode);
}
sai_tunnel_dscp_mode_t getSaiDscpMode(cfg::IpTunnelMode mode) {
  switch (mode) {
    case cfg::IpTunnelMode::UNIFORM:
      return SAI_TUNNEL_DSCP_MODE_UNIFORM_MODEL;
    case cfg::IpTunnelMode::PIPE:
      return SAI_TUNNEL_DSCP_MODE_PIPE_MODEL;
    case cfg::IpTunnelMode::USER:
      break;
  }
  throw FbossError("Failed to convert DSCP mode to SAI type: ", mode);
}
sai_tunnel_decap_ecn_mode_t getSaiDecapEcnMode(cfg::IpTunnelMode mode) {
  switch (mode) {
    case cfg::IpTunnelMode::UNIFORM:
      return SAI_TUNNEL_DECAP_ECN_MODE_STANDARD;
    case cfg::IpTunnelMode::PIPE:
      return SAI_TUNNEL_DECAP_ECN_MODE_COPY_FROM_OUTER;
    case cfg::IpTunnelMode::USER:
      return SAI_TUNNEL_DECAP_ECN_MODE_USER_DEFINED;
  }
  throw FbossError("Failed to convert ECN mode to SAI type: ", mode);
}
} // namespace

SaiTunnelManager::SaiTunnelManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* /*platform*/)
    : saiStore_(saiStore), managerTable_(managerTable) {}

SaiTunnelManager::~SaiTunnelManager() {}

std::shared_ptr<SaiP2MPTunnelTerm> SaiTunnelManager::addP2MPTunnelTerm(
    const std::shared_ptr<IpTunnel>& swTunnel,
    TunnelSaiId tunnelSaiId) {
  auto& tunnelTermStore = saiStore_->get<SaiP2MPTunnelTermTraits>();
  SaiVirtualRouterHandle* virtualRouterHandle =
      managerTable_->virtualRouterManager().getVirtualRouterHandle(RouterID(0));
  SaiP2MPTunnelTermTraits::CreateAttributes k2{
      getSaiTunnelTermType(swTunnel->getTunnelTermType()),
      virtualRouterHandle->virtualRouter->adapterKey(),
      swTunnel->getSrcIP(), // Term Dest Ip
      getSaiTunnelType(swTunnel->getType()), // Tunnel Type
      tunnelSaiId};

  return tunnelTermStore.setObject(k2, k2);
}

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
  if (swTunnel->getTunnelTermType() == cfg::TunnelTerminationType::P2MP) {
    auto tunnelHandle = std::make_unique<SaiTunnelHandle>();
    tunnelHandle->tunnel = tunnelObj;
    tunnelHandle->tunnelTerm =
        addP2MPTunnelTerm(swTunnel, tunnelObj->adapterKey());
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
} // namespace facebook::fboss
