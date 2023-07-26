// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/switch/SaiLagManager.h"

#include "fboss/agent/hw/HwPortFb303Stats.h"
#include "fboss/agent/hw/HwTrunkCounters.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/hw/sai/switch/ConcurrentIndices.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiVlanManager.h"

#include "fboss/agent/hw/sai/store/SaiStore.h"

#include <folly/container/Enumerate.h>

namespace facebook::fboss {

LagSaiId SaiLagManager::addLag(
    const std::shared_ptr<AggregatePort>& aggregatePort) {
  XLOG(DBG2) << "adding aggregate port : " << aggregatePort->getID();

  auto name = aggregatePort->getName();
  std::array<char, 32> labelValue{};
  for (auto i = 0; i < 32 && i < name.length(); i++) {
    labelValue[i] = name[i];
  }
  // TODO(pshaikh): support LAG without ports?
  auto subports = aggregatePort->sortedSubports();
  CHECK_GT(subports.size(), 0);
  auto& subPort = subports.front();
  auto portSaiIdsIter = concurrentIndices_->portSaiIds.find(subPort.portID);
  // port must exist before LAG
  CHECK(portSaiIdsIter != concurrentIndices_->portSaiIds.end());
  auto portSaiId = portSaiIdsIter->second;
  // port must be part of some VLAN and all members of same LAG are part of same
  // VLAN
  auto vlanSaiIdsIter =
      concurrentIndices_->vlanIds.find(PortDescriptorSaiId(portSaiId));
  CHECK(vlanSaiIdsIter != concurrentIndices_->vlanIds.end());

  auto vlanID = vlanSaiIdsIter->second;

  SaiLagTraits::CreateAttributes createAttributes{labelValue, vlanID};

  auto& lagStore = saiStore_->get<SaiLagTraits>();
  auto lag = lagStore.setObject(
      SaiLagTraits::Attributes::Label{labelValue}, createAttributes);
  LagSaiId lagSaiId = lag->adapterKey();
  std::map<PortSaiId, std::shared_ptr<SaiLagMember>> members;
  for (auto iter : folly::enumerate(aggregatePort->subportAndFwdState())) {
    auto [subPort, fwdState] = *iter;
    auto member = addMember(lag, aggregatePort->getID(), subPort, fwdState);
    members.emplace(std::move(member));
  }
  concurrentIndices_->vlanIds.emplace(
      PortDescriptorSaiId(lag->adapterKey()), vlanID);
  concurrentIndices_->aggregatePortIds.emplace(
      lag->adapterKey(), aggregatePort->getID());
  auto handle = std::make_unique<SaiLagHandle>();

  handle->members = std::move(members);
  handle->lag = std::move(lag);
  handle->minimumLinkCount = aggregatePort->getMinimumLinkCount();
  handle->vlanId = vlanID;
  handle->counters = std::make_unique<utility::HwTrunkCounters>(
      aggregatePort->getID(), aggregatePort->getName());
  handles_.emplace(aggregatePort->getID(), std::move(handle));
  managerTable_->vlanManager().createVlanMember(
      vlanID, SaiPortDescriptor(aggregatePort->getID()));

  return lagSaiId;
}

void SaiLagManager::removeLag(
    const std::shared_ptr<AggregatePort>& aggregatePort) {
  XLOG(DBG2) << "removing aggregate port : " << aggregatePort->getID();
  auto iter = handles_.find(aggregatePort->getID());
  if (iter == handles_.end()) {
    throw FbossError(
        "attempting to non-existing remove LAG ", aggregatePort->getID());
  }
  removeLagHandle(iter->first, iter->second.get());
  handles_.erase(iter->first);
}

void SaiLagManager::changeLag(
    const std::shared_ptr<AggregatePort>& oldAggregatePort,
    const std::shared_ptr<AggregatePort>& newAggregatePort) {
  auto handleIter = handles_.find(oldAggregatePort->getID());
  CHECK(handleIter != handles_.end());
  auto& saiLagHandle = handleIter->second;

  saiLagHandle->minimumLinkCount = newAggregatePort->getMinimumLinkCount();

  auto oldPortAndFwdState = oldAggregatePort->subportAndFwdState();
  auto newPortAndFwdState = newAggregatePort->subportAndFwdState();
  auto oldIter = oldPortAndFwdState.begin();
  auto newIter = newPortAndFwdState.begin();

  while (oldIter != oldPortAndFwdState.end() &&
         newIter != newPortAndFwdState.end()) {
    if (oldIter->first < newIter->first) {
      removeMember(oldAggregatePort->getID(), oldIter->first);
      oldIter++;
    } else if (newIter->first < oldIter->first) {
      // add member
      auto member = addMember(
          saiLagHandle->lag, newAggregatePort->getID(), newIter->first);
      setMemberState(member.second.get(), newIter->second);
      saiLagHandle->members.emplace(std::move(member));
      newIter++;
    } else {
      if (oldIter->second != newIter->second) {
        // forwarding state changed
        auto member = getMember(saiLagHandle.get(), newIter->first);
        setMemberState(member, newIter->second);
      }
      newIter++;
      oldIter++;
    }
  }

  while (oldIter != oldPortAndFwdState.end()) {
    removeMember(oldAggregatePort->getID(), oldIter->first);
    oldIter++;
  }
  while (newIter != newPortAndFwdState.end()) {
    auto member =
        addMember(saiLagHandle->lag, newAggregatePort->getID(), newIter->first);
    setMemberState(member.second.get(), newIter->second);
    saiLagHandle->members.emplace(std::move(member));
    newIter++;
  }
  if (newAggregatePort->getName() != oldAggregatePort->getName()) {
    saiLagHandle->counters->reinitialize(
        newAggregatePort->getID(), newAggregatePort->getName());
  }
}

std::pair<PortSaiId, std::shared_ptr<SaiLagMember>> SaiLagManager::addMember(
    const std::shared_ptr<SaiLag>& lag,
    AggregatePortID aggregatePortID,
    PortID subPort,
    AggregatePort::Forwarding fwdState) {
  auto portHandle = managerTable_->portManager().getPortHandle(subPort);
  CHECK(portHandle);
  portHandle->bridgePort.reset();
  auto saiPortId = portHandle->port->adapterKey();
  auto saiLagId = lag->adapterKey();

  SaiLagMemberTraits::AdapterHostKey adapterHostKey{saiLagId, saiPortId};
  auto egressDisabled = (fwdState == AggregatePort::Forwarding::ENABLED)
      ? SaiLagMemberTraits::Attributes::EgressDisable{false}
      : SaiLagMemberTraits::Attributes::EgressDisable{true};
  SaiLagMemberTraits::CreateAttributes attrs{
      saiLagId, saiPortId, egressDisabled};
  auto& lagMemberStore = saiStore_->get<SaiLagMemberTraits>();
  auto member = lagMemberStore.setObject(adapterHostKey, attrs);
  concurrentIndices_->memberPort2AggregatePortIds.emplace(
      saiPortId, aggregatePortID);
  return {saiPortId, member};
}

void SaiLagManager::removeMember(AggregatePortID aggPort, PortID subPort) {
  auto handlesIter = handles_.find(aggPort);
  CHECK(handlesIter != handles_.end());
  auto portHandle = managerTable_->portManager().getPortHandle(subPort);
  CHECK(portHandle);
  auto saiPortId = portHandle->port->adapterKey();
  auto membersIter = handlesIter->second->members.find(saiPortId);
  if (membersIter == handlesIter->second->members.end()) {
    // link down will remove lag member, resulting in LACP machine processing
    // lag shrink. this  will also cause LACP machine to issue state delta to
    // remove  lag member. so  ignore the lag member  removal which  could be
    // issued second time by sw switch.
    XLOG(DBG6) << "member " << subPort << " of aggregate port " << aggPort
               << " was already removed.";
    return;
  }
  membersIter->second.reset();
  handlesIter->second->members.erase(membersIter);
  concurrentIndices_->memberPort2AggregatePortIds.erase(saiPortId);
  // During rollback, all hw calls are skipped except creation will fail.
  // For lags that are already created, rollback will not remove any of
  // those in hardware. Lag and lag members will be reclaim when the
  // lastGoodState is applied. Therefore, there's no need to create bridge port
  // in this case.
  if (getHwWriteBehavior() != HwWriteBehavior::SKIP) {
    portHandle->bridgePort = managerTable_->bridgeManager().addBridgePort(
        SaiPortDescriptor(subPort),
        PortDescriptorSaiId(portHandle->port->adapterKey()));
  }
}

SaiLagHandle* FOLLY_NULLABLE
SaiLagManager::getLagHandleIf(AggregatePortID aggregatePortID) const {
  auto iter = handles_.find(aggregatePortID);
  if (iter == handles_.end()) {
    return nullptr;
  }
  return iter->second.get();
}

SaiLagHandle* SaiLagManager::getLagHandle(
    AggregatePortID aggregatePortID) const {
  auto* handle = getLagHandleIf(aggregatePortID);
  if (handle) {
    return handle;
  }
  throw FbossError("handle for aggregate port ", aggregatePortID, " not found");
}

bool SaiLagManager::isMinimumLinkMet(AggregatePortID aggregatePortID) const {
  const auto* handle = getLagHandle(aggregatePortID);
  return handle->minimumLinkCount <= getActiveMemberCount(aggregatePortID);
}

void SaiLagManager::removeLagHandle(
    AggregatePortID aggPort,
    SaiLagHandle* handle) {
  // remove members
  while (!handle->members.empty()) {
    auto membersIter = handle->members.begin();
    auto portSaiId = membersIter->first;
    auto iter = concurrentIndices_->portIds.find(portSaiId);
    CHECK(iter != concurrentIndices_->portIds.end());
    removeMember(aggPort, iter->second);
  }
  // remove bridge port
  handle->bridgePort.reset();
  managerTable_->vlanManager().removeVlanMember(
      handle->vlanId, SaiPortDescriptor(aggPort));
  concurrentIndices_->vlanIds.erase(
      PortDescriptorSaiId(handle->lag->adapterKey()));
  concurrentIndices_->aggregatePortIds.erase(handle->lag->adapterKey());
  // remove lag
  handle->lag.reset();
}

SaiLagManager::~SaiLagManager() {
  for (auto& handlesIter : handles_) {
    auto& [aggPortID, handle] = handlesIter;
    removeLagHandle(aggPortID, handle.get());
  }
}

size_t SaiLagManager::getLagMemberCount(AggregatePortID aggPort) const {
  auto handle = getLagHandle(aggPort);
  return handle->members.size();
}

size_t SaiLagManager::getActiveMemberCount(AggregatePortID aggPort) const {
  const auto* handle = getLagHandle(aggPort);
  return std::count_if(
      std::begin(handle->members),
      std::end(handle->members),
      [](const auto& entry) {
        auto member = entry.second;
        auto attributes = member->attributes();
        auto isEgressDisabled =
            std::get<SaiLagMemberTraits::Attributes::EgressDisable>(attributes)
                .value();
        return !isEgressDisabled;
      });
}

void SaiLagManager::setMemberState(
    SaiLagMember* member,
    AggregatePort::Forwarding fwdState) {
  switch (fwdState) {
    case AggregatePort::Forwarding::DISABLED:
      member->setAttribute(SaiLagMemberTraits::Attributes::EgressDisable{true});
      break;
    case AggregatePort::Forwarding::ENABLED:
      member->setAttribute(
          SaiLagMemberTraits::Attributes::EgressDisable{false});
      break;
  }
}

SaiLagMember* SaiLagManager::getMember(SaiLagHandle* handle, PortID port) {
  auto portsIter = concurrentIndices_->portSaiIds.find(port);
  if (portsIter == concurrentIndices_->portSaiIds.end()) {
    throw FbossError("port sai id not found for lag member port ", port);
  }
  auto membersIter = handle->members.find(portsIter->second);
  if (membersIter == handle->members.end()) {
    throw FbossError("member not found for lag member port ", port);
  }
  return membersIter->second.get();
}

void SaiLagManager::disableMember(AggregatePortID aggPort, PortID subPort) {
  auto handleIter = handles_.find(aggPort);
  CHECK(handleIter != handles_.end());
  auto& saiLagHandle = handleIter->second;
  auto member = getMember(saiLagHandle.get(), subPort);
  setMemberState(member, AggregatePort::Forwarding::DISABLED);
}

bool SaiLagManager::isLagMember(PortID port) {
  auto handle = managerTable_->portManager().getPortHandle(port);
  if (!handle || !handle->port) {
    throw FbossError("lag membership checked for unknown port ", port);
  }
  return concurrentIndices_->memberPort2AggregatePortIds.find(
             handle->port->adapterKey()) !=
      concurrentIndices_->memberPort2AggregatePortIds.end();
}

void SaiLagManager::addBridgePort(
    const std::shared_ptr<AggregatePort>& aggPort) {
  auto handle = getLagHandle(aggPort->getID());
  auto& lag = handle->lag;
  handle->bridgePort = managerTable_->bridgeManager().addBridgePort(
      SaiPortDescriptor(aggPort->getID()),
      PortDescriptorSaiId(lag->adapterKey()));
}

void SaiLagManager::changeBridgePort(
    const std::shared_ptr<AggregatePort>& /*oldPort*/,
    const std::shared_ptr<AggregatePort>& newPort) {
  return addBridgePort(newPort);
}

void SaiLagManager::updateStats(AggregatePortID aggPort) {
  HwTrunkStats stats{};
  utility::clearHwTrunkStats(stats);

  std::optional<std::chrono::seconds> timeRetrieved{};

  auto* handle = getLagHandle(aggPort);
  for (auto member : handle->members) {
    auto portSaiId = member.first;
    auto portIdsIter = concurrentIndices_->portIds.find(portSaiId);
    if (portIdsIter == concurrentIndices_->portIds.end()) {
      continue;
    }
    auto fb303Stats =
        managerTable_->portManager().getLastPortStat(portIdsIter->second);
    if (!fb303Stats) {
      continue;
    }
    utility::accumulateHwTrunkMemberStats(stats, fb303Stats->portStats());
    if (!timeRetrieved) {
      timeRetrieved = fb303Stats->timeRetrieved();
    }
  }
  if (timeRetrieved) {
    // at least one member exists in lag
    handle->counters->updateCounters(timeRetrieved.value(), stats);
  }
}

std::optional<HwTrunkStats> SaiLagManager::getHwTrunkStats(
    AggregatePortID aggPort) const {
  auto* handle = getLagHandleIf(aggPort);
  if (!handle) {
    return std::nullopt;
  }
  return handle->counters->getHwTrunkStats();
}
} // namespace facebook::fboss
