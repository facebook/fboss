/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestFlowletSwitchingUtils.h"
#include <gtest/gtest.h>

namespace facebook::fboss {
class TestEnsembleIf;
}

namespace facebook::fboss::utility {

bool validateFlowletSwitchingEnabled(
    const HwSwitch* /* unused */,
    const cfg::FlowletSwitchingConfig& /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
  return false;
}

bool verifyEcmpForFlowletSwitching(
    const HwSwitch* /* unused */,
    const folly::CIDRNetwork& /* unused */,
    const cfg::FlowletSwitchingConfig& /* unused */,
    const cfg::PortFlowletConfig& /* unused */,
    bool /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
  return false;
}

bool verifyEcmpForNonFlowlet(
    const HwSwitch* /* unused */,
    const folly::CIDRNetwork& /* unused */,
    const bool /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
  return false;
}

bool validatePortFlowletQuality(
    const HwSwitch* /* unused */,
    const PortID& /* unused */,
    const cfg::PortFlowletConfig& /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  return false;
}

bool validateFlowletSwitchingDisabled(const HwSwitch* /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  return false;
}

void setEcmpMemberStatus(const TestEnsembleIf* /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
}

bool validateFlowSetTable(
    const HwSwitch* /*unit*/,
    const bool /*expectFlowsetSizeZero*/) {
  EXPECT_TRUE(false);
  return false;
}

int getL3EcmpDlbFailPackets(const TestEnsembleIf* /*ensemble*/) {
  throw FbossError("getL3EcmpDlbFailPackets : unsupported in Sai Switch");
  return 0;
}

} // namespace facebook::fboss::utility
