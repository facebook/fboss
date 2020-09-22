// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPacketTrapEntry.h"

#include "fboss/agent/hw/sai/switch/SaiAclTableManager.h"
#include "fboss/agent/hw/sai/switch/SaiManagerTable.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

namespace {

using namespace facebook::fboss;

std::shared_ptr<AclEntry> getSourcePortTrapAclEntry(PortID port) {
  auto aclEntry = std::make_shared<AclEntry>(1, "AclEntry1");
  aclEntry->setSrcPort(port);
  aclEntry->setActionType(cfg::AclActionType::PERMIT);
  MatchAction matchAction;
  cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
  queueAction.queueId_ref() = 0;
  matchAction.setSendToQueue(std::make_pair(queueAction, true));
  aclEntry->setAclAction(matchAction);
  return aclEntry;
}

} // namespace
namespace facebook::fboss {

HwTestPacketTrapEntry::HwTestPacketTrapEntry(HwSwitch* hwSwitch, PortID port) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch);
  auto aclEntry = getSourcePortTrapAclEntry(port);
  saiSwitch->managerTable()->aclTableManager().addAclEntry(
      aclEntry, SaiSwitch::kAclTable1);
}

HwTestPacketTrapEntry::~HwTestPacketTrapEntry() {}

} // namespace facebook::fboss
