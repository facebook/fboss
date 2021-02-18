// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiLagManager.h"

#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"

#include <folly/container/Enumerate.h>

namespace facebook::fboss {

void SaiLagManager::addLag(
    const std::shared_ptr<AggregatePort>& aggregatePort) {
  XLOG(INFO) << "adding aggregate port : " << aggregatePort->getID();

  auto name = aggregatePort->getName();
  std::array<char, 32> labelValue{};
  for (auto i = 0; i < 32 && i < name.length(); i++) {
    labelValue[i] = name[i];
  }
  auto& lagStore = SaiStore::getInstance()->get<SaiLagTraits>();
  auto lag = lagStore.setObject(
      SaiLagTraits::Attributes::Label{labelValue}, std::tuple<>());
  std::map<PortSaiId, std::shared_ptr<SaiLagMember>> members;
  for (auto iter : folly::enumerate(aggregatePort->subportAndFwdState())) {
    auto [subPort, fwdState] = *iter;
    if (fwdState == AggregatePort::Forwarding::DISABLED) {
      // subport disabled for forwarding
      continue;
    }
    members.emplace(addMember(lag, subPort));
  }

  auto handle = std::make_unique<SaiLagHandle>();
  // create bridge port for LAG
  handle->bridgePort = managerTable_->bridgeManager().addBridgePort(
      SaiPortDescriptor(aggregatePort->getID()),
      PortDescriptorSaiId(lag->adapterKey()));
  handle->members = std::move(members);
  handle->lag = std::move(lag);
  handles_.emplace(aggregatePort->getID(), std::move(handle));
}

void SaiLagManager::removeLag(
    const std::shared_ptr<AggregatePort>& aggregatePort) {
  XLOG(INFO) << "removing aggregate port : " << aggregatePort->getID();
  auto iter = handles_.find(aggregatePort->getID());
  if (iter == handles_.end()) {
    throw FbossError(
        "attempting to non-existing remove LAG ", aggregatePort->getID());
  }
  iter->second.reset();
  handles_.erase(iter->first);
}

void SaiLagManager::changeLag(
    const std::shared_ptr<AggregatePort>& oldAggregatePort,
    const std::shared_ptr<AggregatePort>& newAggregatePort) {
  // TODO(pshaikh): do better
  removeLag(oldAggregatePort);
  addLag(newAggregatePort);
}

std::pair<PortSaiId, std::shared_ptr<SaiLagMember>> SaiLagManager::addMember(
    const std::shared_ptr<SaiLag>& lag,
    PortID subPort) {
  auto portHandle = managerTable_->portManager().getPortHandle(subPort);
  CHECK(portHandle);
  auto saiPortId = portHandle->port->adapterKey();
  auto saiLagId = lag->adapterKey();

  SaiLagMemberTraits::CreateAttributes attrs{saiLagId, saiPortId};
  auto& lagMemberStore = SaiStore::getInstance()->get<SaiLagMemberTraits>();
  return {saiPortId, lagMemberStore.setObject(attrs, attrs)};
}

void SaiLagManager::removeMember(AggregatePortID aggPort, PortID subPort) {
  auto iter = handles_.find(aggPort);
  CHECK(iter != handles_.end());
  auto portHandle = managerTable_->portManager().getPortHandle(subPort);
  CHECK(portHandle);
  auto saiPortId = portHandle->port->adapterKey();
  iter->second->members.erase(saiPortId);
}
} // namespace facebook::fboss
