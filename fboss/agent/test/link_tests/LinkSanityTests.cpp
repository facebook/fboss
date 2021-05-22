// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/test/link_tests/LinkTest.h"

using namespace ::testing;
using namespace facebook::fboss;

class LinkSanityTests : public LinkTest {};

// Tests that the link comes up after a flap on the ASIC
TEST_F(LinkSanityTests, asicLinkFlap) {
  auto ports = getCabledPorts();
  // Set the port status on all cabled ports to false. The link should go down
  for (const auto& port : ports) {
    setPortStatus(port, false);
  }
  EXPECT_NO_THROW(waitForAllCabledPorts(false));

  // Set the port status on all cabled ports to true. The link should come back
  // up
  for (const auto& port : ports) {
    setPortStatus(port, true);
  }
  EXPECT_NO_THROW(waitForAllCabledPorts(true));
}
