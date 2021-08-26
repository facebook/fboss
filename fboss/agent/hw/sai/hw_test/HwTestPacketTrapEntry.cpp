// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace {

using namespace facebook::fboss;

std::shared_ptr<AclEntry> getTrapAclEntry(
    bool srcPort,
    std::optional<PortID> port,
    std::optional<folly::CIDRNetwork> dstPrefix,
    int priority) {
  auto aclEntry = std::make_shared<AclEntry>(
      priority, "AclEntry" + folly::to<std::string>(priority));
  srcPort ? aclEntry->setSrcPort(port.value())
          : aclEntry->setDstIp(dstPrefix.value());
  aclEntry->setActionType(cfg::AclActionType::PERMIT);
  MatchAction matchAction;
  cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
  queueAction.queueId_ref() = 0;
  matchAction.setSendToQueue(std::make_pair(queueAction, true));
  matchAction.setToCpuAction(cfg::ToCpuAction::COPY);
  aclEntry->setAclAction(matchAction);
  return aclEntry;
}

} // namespace
namespace facebook::fboss {

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    const std::set<PortID>& ports) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch);
  int priority = 1;
  for (auto port : ports) {
    auto aclEntry = getTrapAclEntry(true, port, std::nullopt, priority++);
    saiSwitch->managerTable()->aclTableManager().addAclEntry(
        aclEntry, SaiSwitch::kAclTable1);
  }
}

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    const std::set<folly::CIDRNetwork>& dstPrefixes) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch);
  int priority = 1;
  for (const auto& dstPrefix : dstPrefixes) {
    auto aclEntry = getTrapAclEntry(false, std::nullopt, dstPrefix, priority++);
    saiSwitch->managerTable()->aclTableManager().addAclEntry(
        aclEntry, SaiSwitch::kAclTable1);
  }
}

HwTestPacketTrapEntry::~HwTestPacketTrapEntry() {}

} // namespace facebook::fboss
