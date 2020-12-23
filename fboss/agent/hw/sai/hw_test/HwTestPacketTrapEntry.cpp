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
    std::optional<folly::IPAddress> dstIp) {
  auto aclEntry = std::make_shared<AclEntry>(1, "AclEntry1");
  srcPort ? aclEntry->setSrcPort(port.value())
          : aclEntry->setDstIp(folly::CIDRNetwork(dstIp.value(), 128));
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

HwTestPacketTrapEntry::HwTestPacketTrapEntry(HwSwitch* hwSwitch, PortID port) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch);
  auto aclEntry = getTrapAclEntry(true, port, std::nullopt);
  saiSwitch->managerTable()->aclTableManager().addAclEntry(
      aclEntry, SaiSwitch::kAclTable1);
}

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    folly::IPAddress& dstIp) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch);
  auto aclEntry = getTrapAclEntry(false, std::nullopt, dstIp);
  saiSwitch->managerTable()->aclTableManager().addAclEntry(
      aclEntry, SaiSwitch::kAclTable1);
}

HwTestPacketTrapEntry::~HwTestPacketTrapEntry() {}

} // namespace facebook::fboss
