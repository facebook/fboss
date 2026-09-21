// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestTrunkUtils.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include "fboss/agent/hw/sai/hw_test/SaiSwitchEnsemble.h"
#include "fboss/agent/hw/sai/switch/SaiLagManager.h"
#include "fboss/agent/hw/sai/switch/SaiRxPacket.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void verifyAggregatePortCount(const HwSwitchEnsemble* ensemble, uint8_t count) {
  auto* saiSwitchEnsemble = static_cast<const SaiSwitchEnsemble*>(ensemble);
  auto* saiSwitch = saiSwitchEnsemble->getHwSwitch();
  EXPECT_EQ(saiSwitch->managerTable()->lagManager().getLagCount(), count);
}

void verifyAggregatePort(
    const HwSwitchEnsemble* ensemble,
    AggregatePortID aggregatePortID) {
  auto* saiSwitchEnsemble = static_cast<const SaiSwitchEnsemble*>(ensemble);
  auto* saiSwitch = saiSwitchEnsemble->getHwSwitch();
  auto& lagManager = saiSwitch->managerTable()->lagManager();
  EXPECT_NO_THROW(lagManager.getLagHandle(aggregatePortID));
}

void verifyAggregatePortMemberCount(
    const HwSwitchEnsemble* ensemble,
    AggregatePortID aggregatePortID,
    uint8_t totalCount,
    uint8_t currentCount) {
  auto* saiSwitchEnsemble = static_cast<const SaiSwitchEnsemble*>(ensemble);
  auto* saiSwitch = saiSwitchEnsemble->getHwSwitch();
  auto& lagManager = saiSwitch->managerTable()->lagManager();
  EXPECT_EQ(lagManager.getLagMemberCount(aggregatePortID), totalCount);
  EXPECT_EQ(lagManager.getActiveMemberCount(aggregatePortID), currentCount);
}

void verifyPktFromAggregatePort(
    const HwSwitchEnsemble* /*ensemble*/,
    AggregatePortID aggregatePortID) {
  std::array<char, 8> data{};
  // TODO (T159867926): Set the right queue ID once the vendor
  // set the right queue ID in the rx callback.
  auto rxPacket = std::make_unique<SaiRxPacket>(
      data.size(),
      data.data(),
      aggregatePortID,
      VlanID(1),
      cfg::PacketRxReason::UNMATCHED,
      0 /* queue Id */);
  EXPECT_TRUE(rxPacket->isFromAggregatePort());
}

int getTrunkMemberCountInHw(
    const HwSwitch* hw,
    AggregatePortID id,
    int /* countInSw */) {
  auto saiSwitch = static_cast<const SaiSwitch*>(hw);
  auto& lagManager = saiSwitch->managerTable()->lagManager();
  return lagManager.getActiveMemberCount(id);
}
} // namespace facebook::fboss::utility
