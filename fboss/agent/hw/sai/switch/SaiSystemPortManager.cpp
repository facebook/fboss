/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiSystemPortManager.h"

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/sai/store/SaiStore.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/platforms/sai/SaiPlatform.h"

#include <folly/logging/xlog.h>

#include <fmt/ranges.h>

using namespace std::chrono;

namespace facebook::fboss {
namespace {
bool createOnlyAttributesChanged(
    const SaiSystemPortTraits::CreateAttributes& oldAttrs,
    const SaiSystemPortTraits::CreateAttributes& newAttrs) {
  return GET_ATTR(SystemPort, ConfigInfo, oldAttrs) !=
      GET_ATTR(SystemPort, ConfigInfo, newAttrs);
}
} // namespace
SaiSystemPortManager::SaiSystemPortManager(
    SaiStore* saiStore,
    SaiManagerTable* managerTable,
    SaiPlatform* platform,
    ConcurrentIndices* concurrentIndices)
    : saiStore_(saiStore),
      managerTable_(managerTable),
      platform_(platform),
      concurrentIndices_(concurrentIndices) {}

SaiSystemPortTraits::CreateAttributes
SaiSystemPortManager::attributesFromSwSystemPort(
    const std::shared_ptr<SystemPort>& swSystemPort) const {
  sai_system_port_config_t config{
      .port_id = static_cast<uint32_t>(swSystemPort->getID()),
      .attached_switch_id = static_cast<uint32_t>(swSystemPort->getSwitchId()),
      .attached_core_index =
          static_cast<uint32_t>(swSystemPort->getCoreIndex()),
      .attached_core_port_index =
          static_cast<uint32_t>(swSystemPort->getCorePortIndex()),
      .speed = static_cast<uint32_t>(swSystemPort->getSpeedMbps()),
      .num_voq = static_cast<uint32_t>(swSystemPort->getNumVoqs()),
  };
  return SaiSystemPortTraits::CreateAttributes{
      config, swSystemPort->getEnabled(), std::nullopt};
}

SystemPortSaiId SaiSystemPortManager::addSystemPort(
    const std::shared_ptr<SystemPort>& swSystemPort) {
  auto systemPortHandle = getSystemPortHandle(swSystemPort->getID());
  if (systemPortHandle) {
    throw FbossError(
        "Attempted to add systemPort which already exists: ",
        swSystemPort->getID(),
        " SAI id: ",
        systemPortHandle->systemPort->adapterKey());
  }
  SaiSystemPortTraits::CreateAttributes attributes =
      attributesFromSwSystemPort(swSystemPort);

  auto handle = std::make_unique<SaiSystemPortHandle>();

  auto& systemPortStore = saiStore_->get<SaiSystemPortTraits>();
  SaiSystemPortTraits::AdapterHostKey systemPortKey{
      GET_ATTR(SystemPort, ConfigInfo, attributes)};
  auto saiSystemPort = systemPortStore.setObject(
      systemPortKey, attributes, swSystemPort->getID());
  handle->systemPort = saiSystemPort;
  loadQueues(*handle, swSystemPort->getNumVoqs());
  handles_.emplace(swSystemPort->getID(), std::move(handle));
  concurrentIndices_->sysPortIds.insert(
      {saiSystemPort->adapterKey(), swSystemPort->getID()});
  concurrentIndices_->sysPortSaiIds.insert(
      {swSystemPort->getID(), saiSystemPort->adapterKey()});
  return saiSystemPort->adapterKey();
}

void SaiSystemPortManager::changeSystemPort(
    const std::shared_ptr<SystemPort>& oldSystemPort,
    const std::shared_ptr<SystemPort>& newSystemPort) {
  CHECK_EQ(oldSystemPort->getID(), newSystemPort->getID());
  auto handle = getSystemPortHandleImpl(newSystemPort->getID());
  auto newAttributes = attributesFromSwSystemPort(newSystemPort);
  if (createOnlyAttributesChanged(
          handle->systemPort->attributes(), newAttributes)) {
    removeSystemPort(oldSystemPort);
    addSystemPort(newSystemPort);
  } else {
    handle->systemPort->setAttributes(newAttributes);
  }
}

void SaiSystemPortManager::loadQueues(
    SaiSystemPortHandle& sysPortHandle,
    int64_t numVoqs) const {
  if (!platform_->getAsic()->isSupported(HwAsic::Feature::VOQ)) {
    return;
  }
  std::vector<sai_object_id_t> queueList;
  queueList.resize(numVoqs);
  SaiSystemPortTraits::Attributes::QosVoqList queueListAttribute{queueList};
  auto queueSaiIdList =
      SaiApiTable::getInstance()->systemPortApi().getAttribute(
          sysPortHandle.systemPort->adapterKey(), queueListAttribute);
  if (queueSaiIdList.size() == 0) {
    throw FbossError("no queues exist for system port ");
  }
  std::vector<QueueSaiId> queueSaiIds;
  queueSaiIds.reserve(queueSaiIdList.size());
  std::transform(
      queueSaiIdList.begin(),
      queueSaiIdList.end(),
      std::back_inserter(queueSaiIds),
      [](sai_object_id_t queueId) -> QueueSaiId {
        return QueueSaiId(queueId);
      });
  sysPortHandle.queues = managerTable_->queueManager().loadQueues(queueSaiIds);
}

void SaiSystemPortManager::removeSystemPort(
    const std::shared_ptr<SystemPort>& swSystemPort) {
  auto swId = swSystemPort->getID();
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    throw FbossError("Attempted to remove non-existent system port: ", swId);
  }
  concurrentIndices_->sysPortSaiIds.erase(swId);
  concurrentIndices_->sysPortIds.erase(itr->second->systemPort->adapterKey());
  handles_.erase(itr);
  XLOG(DBG2) << "removed system port: " << swId;
}

// private const getter for use by const and non-const getters
SaiSystemPortHandle* SaiSystemPortManager::getSystemPortHandleImpl(
    SystemPortID swId) const {
  auto itr = handles_.find(swId);
  if (itr == handles_.end()) {
    return nullptr;
  }
  if (!itr->second.get()) {
    XLOG(FATAL) << "Invalid null SaiSystemPortHandle for " << swId;
  }
  return itr->second.get();
}

} // namespace facebook::fboss
