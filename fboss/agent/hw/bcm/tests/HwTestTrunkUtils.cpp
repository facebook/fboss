// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestTrunkUtils.h"

#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"
#include "fboss/agent/hw/bcm/tests/BcmTrunkUtils.h"
#include "fboss/agent/types.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void verifyAggregatePortCount(const HwSwitchEnsemble* ensemble, uint8_t count) {
  auto* bcmSwitchEnsemble = static_cast<const BcmSwitchEnsemble*>(ensemble);
  EXPECT_EQ(
      bcmSwitchEnsemble->getHwSwitch()->getTrunkTable()->numTrunkPorts(),
      count);
}

void verifyAggregatePort(
    const HwSwitchEnsemble* ensemble,
    AggregatePortID aggregatePortID) {
  auto* bcmSwitchEnsemble = static_cast<const BcmSwitchEnsemble*>(ensemble);
  auto trunkTable = bcmSwitchEnsemble->getHwSwitch()->getTrunkTable();
  EXPECT_NO_THROW(trunkTable->getBcmTrunkId(aggregatePortID));
}

void verifyAggregatePortMemberCount(
    const HwSwitchEnsemble* ensemble,
    AggregatePortID aggregatePortID,
    uint8_t totalCount,
    uint8_t currentCount) {
  auto* bcmSwitchEnsemble = static_cast<const BcmSwitchEnsemble*>(ensemble);
  auto unit = bcmSwitchEnsemble->getHwSwitch()->getUnit();
  auto trunkTable = bcmSwitchEnsemble->getHwSwitch()->getTrunkTable();
  EXPECT_EQ(
      currentCount,
      utility::getBcmTrunkMemberCount(
          unit, trunkTable->getBcmTrunkId(aggregatePortID), totalCount));
}

void verifyPktFromAggregatePort(
    const HwSwitchEnsemble* ensemble,
    AggregatePortID aggregatePortID) {
  auto* bcmSwitchEnsemble = static_cast<const BcmSwitchEnsemble*>(ensemble);
  auto* bcmSwitch = bcmSwitchEnsemble->getHwSwitch();
  bool usePktIO = bcmSwitch->usePKTIO();
  BcmPacketT bcmPacket;
  bcm_pkt_t bcmPkt;
  auto bcmTrunkId = bcmSwitch->getTrunkTable()->getBcmTrunkId(aggregatePortID);
  bcmPacket.usePktIO = usePktIO;
  if (!usePktIO) {
    bcmPacket.ptrUnion.pkt = &bcmPkt;
    BCM_PKT_ONE_BUF_SETUP(&bcmPkt, nullptr, 0);
    bcmPkt.src_trunk = bcmTrunkId;
    bcmPkt.flags |= BCM_PKT_F_TRUNK;
    bcmPkt.unit = 0;
    FbBcmRxPacket fbRxPkt(bcmPacket, bcmSwitch);
    EXPECT_TRUE(fbRxPkt.isFromAggregatePort());
    EXPECT_EQ(fbRxPkt.getSrcAggregatePort(), aggregatePortID);
  }
  // Note that for PKTIO these tests are not valid.
  // In the non-PKTIO case, the SDK parse the packet and sets up the fields
  // in the bcm_pkt_s, so they can be queried from this data structure.
  // For PKTIO, such information is not available from the packet data
  // structure.
}

int getTrunkMemberCountInHw(
    const HwSwitch* hw,
    AggregatePortID id,
    int countInSw) {
  auto bcmSwitch = static_cast<const BcmSwitch*>(hw);
  return getBcmTrunkMemberCount(
      bcmSwitch->getUnit(),
      bcmSwitch->getTrunkTable()->getBcmTrunkId(id),
      countInSw);
}
} // namespace facebook::fboss::utility
