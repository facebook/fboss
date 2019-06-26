/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/switch/SaiHostifManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitchManager.h"
#include "fboss/agent/state/ControlPlane.h"

namespace facebook {
namespace fboss {

SaiHostifTrapGroup::SaiHostifTrapGroup(
    SaiApiTable* apiTable,
    const HostifApiParameters::Attributes& attributes,
    sai_object_id_t& switchId)
    : apiTable_(apiTable), attributes_(attributes) {
  auto& hostifApi = apiTable_->hostifApi();
  id_ = hostifApi.create(attributes_.attrs(), switchId);
}

SaiHostifTrapGroup::~SaiHostifTrapGroup() {
  auto& hostifApi = apiTable_->hostifApi();
  hostifApi.remove(id_);
}

bool SaiHostifTrapGroup::operator==(const SaiHostifTrapGroup& other) const {
  return attributes_ == other.attributes();
}
bool SaiHostifTrapGroup::operator!=(const SaiHostifTrapGroup& other) const {
  return !(*this == other);
}

std::shared_ptr<SaiHostifTrapGroup> SaiHostifManager::addHostifTrapGroup(
    uint32_t queueId) {
  HostifApiParameters::Attributes attrs{{queueId, folly::none}};
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  auto entry =
      hostifTrapGroups_.refOrEmplace(queueId, apiTable_, attrs, switchId);
  return entry.first;
}

SaiHostifTrap::SaiHostifTrap(
    SaiApiTable* apiTable,
    const HostifApiParameters::MemberAttributes& attributes,
    sai_object_id_t& switchId,
    std::shared_ptr<SaiHostifTrapGroup> trapGroup)
    : apiTable_(apiTable), attributes_(attributes), trapGroup_(trapGroup) {
  auto& hostifApi = apiTable_->hostifApi();
  id_ = hostifApi.createMember(attributes_.attrs(), switchId);
}

SaiHostifTrap::~SaiHostifTrap() {
  auto& hostifApi = apiTable_->hostifApi();
  hostifApi.removeMember(id_);
}

bool SaiHostifTrap::operator==(const SaiHostifTrap& other) const {
  return attributes_ == other.attributes();
}
bool SaiHostifTrap::operator!=(const SaiHostifTrap& other) const {
  return !(*this == other);
}

sai_hostif_trap_type_t SaiHostifManager::packetReasonToHostifTrap(
    cfg::PacketRxReason reason) {
  switch (reason) {
    case cfg::PacketRxReason::ARP:
      return SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST;
    case cfg::PacketRxReason::DHCP:
      return SAI_HOSTIF_TRAP_TYPE_DHCP;
    default:
      throw FbossError("invalid packet reason: ", reason);
  }
}

cfg::PacketRxReason SaiHostifManager::hostifTrapToPacketReason(
    sai_hostif_trap_type_t trapType) {
  switch (trapType) {
    case SAI_HOSTIF_TRAP_TYPE_ARP_REQUEST:
      return cfg::PacketRxReason::ARP;
    case SAI_HOSTIF_TRAP_TYPE_DHCP:
      return cfg::PacketRxReason::DHCP;
    default:
      throw FbossError("invalid trap type: ", trapType);
  }
}

sai_object_id_t SaiHostifManager::incRefOrAddHostifTrapGroup(
    cfg::PacketRxReason trapId,
    uint32_t queueId) {
  auto hostifTrapGroup = addHostifTrapGroup(queueId);
  auto switchId = managerTable_->switchManager().getSwitchSaiId();
  HostifApiParameters::MemberAttributes::PacketAction packetAction{
      SAI_PACKET_ACTION_TRAP};
  HostifApiParameters::MemberAttributes::TrapType trapType{
      packetReasonToHostifTrap(trapId)};
  HostifApiParameters::MemberAttributes::TrapPriority trapPriority{
      SAI_SWITCH_ATTR_ACL_ENTRY_MINIMUM_PRIORITY};
  HostifApiParameters::MemberAttributes::TrapGroup trapGroup{
      hostifTrapGroup->id()};
  HostifApiParameters::MemberAttributes attributes{
      {trapType, packetAction, trapPriority, trapGroup}};
  auto hostifTrap = std::make_unique<SaiHostifTrap>(
      apiTable_, attributes, switchId, hostifTrapGroup);
  auto hostifTrapId = hostifTrap->id();
  hostifTraps_.emplace(trapId, std::move(hostifTrap));
  return hostifTrapId;
}

void SaiHostifManager::removeHostifTrap(cfg::PacketRxReason trapId) {
  hostifTraps_.erase(trapId);
}

void SaiHostifManager::changeHostifTrap(
    cfg::PacketRxReason /* trapId */,
    uint32_t /* newQueueid */) {
  throw FbossError("Not implemented");
}

void SaiHostifManager::processHostifDelta(const StateDelta& delta) {
  auto controlPlaneDelta = delta.getControlPlaneDelta();
  auto& oldRxReasonToCpuQueueMap =
      controlPlaneDelta.getOld()->getRxReasonToQueue();
  auto& newRxReasonToCpuQueueMap =
      controlPlaneDelta.getNew()->getRxReasonToQueue();
  for (auto newRxReasonEntry : newRxReasonToCpuQueueMap) {
    if (oldRxReasonToCpuQueueMap.size() > 0 &&
        oldRxReasonToCpuQueueMap.at(newRxReasonEntry.first)) {
      changeHostifTrap(newRxReasonEntry.first, newRxReasonEntry.second);
    } else {
      incRefOrAddHostifTrapGroup(
          newRxReasonEntry.first, newRxReasonEntry.second);
    }
  }
  for (auto oldRxReasonEntry : oldRxReasonToCpuQueueMap) {
    if (newRxReasonToCpuQueueMap.size() > 0 &&
        !newRxReasonToCpuQueueMap.at(oldRxReasonEntry.first)) {
      removeHostifTrap(oldRxReasonEntry.first);
    }
  }
}

SaiHostifManager::SaiHostifManager(
    SaiApiTable* apiTable,
    SaiManagerTable* managerTable)
    : apiTable_(apiTable), managerTable_(managerTable) {}

SaiHostifManager::~SaiHostifManager() {
  hostifTraps_.clear();
  hostifTrapGroups_.clear();
}

} // namespace fboss
} // namespace facebook
