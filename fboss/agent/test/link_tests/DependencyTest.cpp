// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestEcmpUtils.h"
#include "fboss/agent/test/link_tests/LinkTest.h"

using namespace facebook::fboss;

TEST_F(LinkTest, ecmpShrink) {
  auto setup = [this]() {
    programDefaultRoute(
        getVlanOwningCabledPorts(), sw()->getPlatform()->getLocalMac());
  };
  auto verify = [this]() {
    auto ecmpPorts = getVlanOwningCabledPorts();
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
    EXPECT_EQ(
        utility::getEcmpSizeInHw(
            sw()->getHw(),
            {folly::IPAddress("::"), 0},
            RouterID(0),
            ecmpPorts.size()),
        ecmpPorts.size());

    for (const auto& port : ecmpPorts) {
      setPortStatus(port.phyPortID(), false);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(false));
    EXPECT_EQ(
        utility::getEcmpSizeInHw(
            sw()->getHw(),
            {folly::IPAddress("::"), 0},
            RouterID(0),
            ecmpPorts.size()),
        0);

    for (const auto& port : ecmpPorts) {
      setPortStatus(port.phyPortID(), true);
    }
    EXPECT_NO_THROW(waitForAllCabledPorts(true));
  };

  verifyAcrossWarmBoots(setup, verify);
}
