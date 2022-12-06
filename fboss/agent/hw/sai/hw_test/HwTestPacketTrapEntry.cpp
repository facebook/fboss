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
    int priority,
    bool counter = false) {
  std::string aclName = "AclEntry" + folly::to<std::string>(priority);
  auto aclEntry = std::make_shared<AclEntry>(priority, std::string(aclName));
  srcPort ? aclEntry->setSrcPort(port.value())
          : aclEntry->setDstIp(dstPrefix.value());
  aclEntry->setActionType(cfg::AclActionType::PERMIT);
  MatchAction matchAction;
  cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
  queueAction.queueId() = 0;
  matchAction.setSendToQueue(std::make_pair(queueAction, true));
  matchAction.setToCpuAction(cfg::ToCpuAction::COPY);
  if (counter) {
    auto trafficCounter = cfg::TrafficCounter();
    trafficCounter.name() = aclName + "-counter";
    trafficCounter.types() = {cfg::CounterType::PACKETS};
    matchAction.setTrafficCounter(trafficCounter);
  }
  aclEntry->setAclAction(matchAction);
  return aclEntry;
}

} // namespace
namespace facebook::fboss {

int getNextFreePriority(SaiSwitch* saiSwitch, const std::string& aclTableName) {
  int offset = 0;
  auto priority = static_cast<int>(
      saiSwitch->managerTable()->aclTableManager().aclEntryCount(aclTableName));
  auto kMaxPriorityOffset = 100;
  /*
   * Find an unused priority which is > than the number of entries in the
   * ACL table. We expect the entries to have consecutive priorities and we
   * should be able to find an unused priority around the number of ACL
   * entries. However, keeping a max offset we should declare failure at
   * as kMaxPriorityOffset.
   */
  while (saiSwitch->managerTable()->aclTableManager().hasAclEntryWithPriority(
             aclTableName, priority + offset) &&
         offset < kMaxPriorityOffset) {
    offset++;
  }
  if (offset == kMaxPriorityOffset) {
    throw FbossError(
        "No free priority in the vicinity of the number of ACL table entries: ",
        priority,
        ". Unexpected!");
  }
  return priority + offset;
}

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    const uint16_t l4DstPort)
    : hwSwitch_(hwSwitch) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch_);
  int priority =
      saiSwitch->managerTable()->aclTableManager().aclEntryCount(kAclTable1);

  auto aclEntry = std::make_shared<AclEntry>(
      priority, std::string("AclEntry" + folly::to<std::string>(priority)));
  aclEntry->setL4DstPort(l4DstPort);
  aclEntry->setActionType(cfg::AclActionType::PERMIT);
  MatchAction matchAction;
  cfg::QueueMatchAction queueAction = cfg::QueueMatchAction();
  queueAction.queueId() = 0;
  matchAction.setSendToQueue(std::make_pair(queueAction, true));
  matchAction.setToCpuAction(cfg::ToCpuAction::COPY);
  aclEntry->setAclAction(matchAction);

  saiSwitch->managerTable()->aclTableManager().addAclEntry(
      aclEntry, kAclTable1);
  aclEntries_.push_back(aclEntry);
}

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    const std::set<PortID>& ports)
    : hwSwitch_(hwSwitch) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch_);
  int priority = getNextFreePriority(saiSwitch, kAclTable1);
  for (auto port : ports) {
    auto aclEntry = getTrapAclEntry(true, port, std::nullopt, priority++);
    saiSwitch->managerTable()->aclTableManager().addAclEntry(
        aclEntry, kAclTable1);
    aclEntries_.push_back(aclEntry);
  }
}

HwTestPacketTrapEntry::HwTestPacketTrapEntry(
    HwSwitch* hwSwitch,
    const std::set<folly::CIDRNetwork>& dstPrefixes)
    : hwSwitch_(hwSwitch) {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch_);
  int priority = getNextFreePriority(saiSwitch, kAclTable1);
  for (const auto& dstPrefix : dstPrefixes) {
    auto aclEntry = getTrapAclEntry(false, std::nullopt, dstPrefix, priority++);
    saiSwitch->managerTable()->aclTableManager().addAclEntry(
        aclEntry, kAclTable1);
    aclEntries_.push_back(aclEntry);
  }
}

HwTestPacketTrapEntry::~HwTestPacketTrapEntry() {
  auto saiSwitch = static_cast<SaiSwitch*>(hwSwitch_);
  for (const auto& aclEntry : aclEntries_) {
    saiSwitch->managerTable()->aclTableManager().removeAclEntry(
        aclEntry, kAclTable1);
  }
}

} // namespace facebook::fboss
