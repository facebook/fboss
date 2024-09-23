/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiMirrorManager.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSystemPortManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

namespace facebook::fboss {

SaiMirrorHandle::SaiMirror SaiMirrorManager::addNodeSpan(
    sai_object_id_t monitorPort) {
  SaiLocalMirrorTraits::AdapterHostKey k{
      SAI_MIRROR_SESSION_TYPE_LOCAL, monitorPort};
  SaiLocalMirrorTraits::CreateAttributes attributes = k;
  auto& store = saiStore_->get<SaiLocalMirrorTraits>();
  return store.setObject(k, attributes);
}

SaiMirrorHandle::SaiMirror SaiMirrorManager::addNodeErSpan(
    const std::shared_ptr<Mirror>& mirror,
    sai_object_id_t monitorPort) {
  auto mirrorTunnel = mirror->getMirrorTunnel().value();
  auto headerVersion = mirrorTunnel.srcIp.isV4() ? 4 : 6;
  auto truncateSize =
      mirror->getTruncate() ? platform_->getAsic()->getMirrorTruncateSize() : 0;
  SaiEnhancedRemoteMirrorTraits::CreateAttributes attributes{
      SAI_MIRROR_SESSION_TYPE_ENHANCED_REMOTE,
      monitorPort,
      SAI_ERSPAN_ENCAPSULATION_TYPE_MIRROR_L3_GRE_TUNNEL,
      mirror->getDscp(),
      mirrorTunnel.srcIp,
      mirrorTunnel.dstIp,
      mirrorTunnel.srcMac,
      mirrorTunnel.dstMac,
      mirrorTunnel.greProtocol,
      headerVersion,
      mirrorTunnel.ttl,
      truncateSize,
      mirror->getSamplingRate()};
  SaiEnhancedRemoteMirrorTraits::AdapterHostKey k{
      SAI_MIRROR_SESSION_TYPE_ENHANCED_REMOTE,
      monitorPort,
      mirrorTunnel.srcIp,
      mirrorTunnel.dstIp};
  auto& store = saiStore_->get<SaiEnhancedRemoteMirrorTraits>();
  return store.setObject(k, attributes);
}

SaiMirrorHandle::SaiMirror SaiMirrorManager::addNodeSflow(
    const std::shared_ptr<Mirror>& mirror,
    sai_object_id_t monitorPort) {
  auto mirrorTunnel = mirror->getMirrorTunnel().value();
  auto headerVersion = mirrorTunnel.srcIp.isV4() ? 4 : 6;
  auto truncateSize =
      mirror->getTruncate() ? platform_->getAsic()->getMirrorTruncateSize() : 0;
  SaiSflowMirrorTraits::CreateAttributes attributes{
      SAI_MIRROR_SESSION_TYPE_SFLOW,
      monitorPort,
      mirror->getDscp(),
      mirrorTunnel.srcIp,
      mirrorTunnel.dstIp,
      mirrorTunnel.srcMac,
      mirrorTunnel.dstMac,
      mirrorTunnel.udpPorts.value().udpSrcPort,
      mirrorTunnel.udpPorts.value().udpDstPort,
      headerVersion,
      mirrorTunnel.ttl,
      truncateSize,
      mirror->getSamplingRate()};
  SaiSflowMirrorTraits::AdapterHostKey k{
      SAI_MIRROR_SESSION_TYPE_SFLOW,
      monitorPort,
      mirrorTunnel.srcIp,
      mirrorTunnel.dstIp,
      mirrorTunnel.udpPorts.value().udpSrcPort,
      mirrorTunnel.udpPorts.value().udpDstPort};
  auto& store = saiStore_->get<SaiSflowMirrorTraits>();
  return store.setObject(k, attributes);
}

SaiMirrorHandle::~SaiMirrorHandle() {
  managerTable->portManager().programMirrorOnAllPorts(
      mirrorId, MirrorAction::STOP);
  managerTable->aclTableManager().programMirrorOnAllAcls(
      mirrorId, MirrorAction::STOP);
}

void SaiMirrorManager::addNode(const std::shared_ptr<Mirror>& mirror) {
  if (!mirror->isResolved()) {
    return;
  }
  auto mirrorHandleIter = mirrorHandles_.find(mirror->getID());
  if (mirrorHandleIter != mirrorHandles_.end()) {
    throw FbossError(
        "Attempted to add mirror which already exists: ", mirror->getID());
  }
  auto mirrorHandle =
      std::make_unique<SaiMirrorHandle>(mirror->getID(), managerTable_);
  auto monitorPort = mirror->getEgressPortDesc().has_value()
      ? getMonitorPort(mirror->getEgressPortDesc().value())
      : getMonitorPort(PortDescriptor(mirror->getEgressPort().value()));
  if (mirror->getMirrorTunnel().has_value()) {
    auto mirrorTunnel = mirror->getMirrorTunnel().value();
    if (mirrorTunnel.udpPorts.has_value()) {
      mirrorHandle->mirror = addNodeSflow(mirror, monitorPort);
    } else {
      mirrorHandle->mirror = addNodeErSpan(mirror, monitorPort);
    }
  } else {
    mirrorHandle->mirror = addNodeSpan(monitorPort);
  }
  mirrorHandles_.emplace(mirror->getID(), std::move(mirrorHandle));
  managerTable_->portManager().programMirrorOnAllPorts(
      mirror->getID(), MirrorAction::START);
  managerTable_->aclTableManager().programMirrorOnAllAcls(
      mirror->getID(), MirrorAction::START);
}

void SaiMirrorManager::removeMirror(const std::shared_ptr<Mirror>& mirror) {
  auto mirrorHandleIter = mirrorHandles_.find(mirror->getID());
  if (mirrorHandleIter == mirrorHandles_.end()) {
    if (!mirror->isResolved()) {
      return;
    }
    throw FbossError(
        "Attempted to remove non-existent mirror: ", mirror->getID());
  }
  managerTable_->portManager().programMirrorOnAllPorts(
      mirror->getID(), MirrorAction::STOP);
  managerTable_->aclTableManager().programMirrorOnAllAcls(
      mirror->getID(), MirrorAction::STOP);
  mirrorHandles_.erase(mirrorHandleIter);
}

void SaiMirrorManager::changeMirror(
    const std::shared_ptr<Mirror>& oldMirror,
    const std::shared_ptr<Mirror>& newMirror) {
  removeMirror(oldMirror);
  addNode(newMirror);
}

SaiMirrorHandle* FOLLY_NULLABLE
SaiMirrorManager::getMirrorHandleImpl(const std::string& mirrorId) const {
  auto itr = mirrorHandles_.find(mirrorId);
  if (itr == mirrorHandles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiMirrorHandle for " << mirrorId;
  }
  return itr->second.get();
}

const SaiMirrorHandle* FOLLY_NULLABLE
SaiMirrorManager::getMirrorHandle(const std::string& mirrorId) const {
  return getMirrorHandleImpl(mirrorId);
}

SaiMirrorHandle* FOLLY_NULLABLE
SaiMirrorManager::getMirrorHandle(const std::string& mirrorId) {
  return getMirrorHandleImpl(mirrorId);
}

SaiMirrorManager::SaiMirrorManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform)
    : saiStore_(saiStore), managerTable_(managerTable), platform_(platform) {}

std::vector<MirrorSaiId> SaiMirrorManager::getAllMirrorSessionOids() const {
  std::vector<MirrorSaiId> mirrorSaiIds;
  for (auto& mirrorIdAndHandle : mirrorHandles_) {
    mirrorSaiIds.push_back(mirrorIdAndHandle.second->adapterKey());
  }
  return mirrorSaiIds;
}

sai_object_id_t SaiMirrorManager::getMonitorPort(
    const PortDescriptor& portDesc) {
  sai_object_id_t monitorPort;
  switch (portDesc.type()) {
    case PortDescriptor::PortType::PHYSICAL: {
      auto portHandle =
          managerTable_->portManager().getPortHandle(portDesc.phyPortID());
      if (!portHandle) {
        throw FbossError(
            "Failed to find sai port for egress port for mirroring: ",
            portDesc.phyPortID());
      }
      monitorPort = portHandle->port->adapterKey();
      break;
    }
    case PortDescriptor::PortType::SYSTEM_PORT: {
      auto systemPortHandle =
          managerTable_->systemPortManager().getSystemPortHandle(
              portDesc.sysPortID());
      if (!systemPortHandle) {
        throw FbossError(
            "Failed to find sai system port for egress port for mirroring: ",
            portDesc.sysPortID());
      }
      monitorPort = systemPortHandle->systemPort->adapterKey();
      break;
    }
    case PortDescriptor::PortType::AGGREGATE: {
      throw FbossError("Invalid agg port desc type received for mirroring");
    }
  }
  return monitorPort;
}

} // namespace facebook::fboss
