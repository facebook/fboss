/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/test/HwTestTeFlowUtils.h"
#include <gtest/gtest.h>

namespace facebook::fboss::utility {

bool validateTeFlowGroupEnabled(
    const HwSwitch* /* unused */,
    int /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
  return false;
}

int getNumTeFlowEntries(const HwSwitch* /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
  return 0;
}

void checkSwHwTeFlowMatch(
    const HwSwitch* /* unused */,
    std::shared_ptr<SwitchState> /* unused */,
    TeFlow /* unused */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
}

uint64_t getTeFlowOutBytes(
    const HwSwitch* /* hw */,
    std::string /* statName */) {
  // This function is not implemented yet.
  // If the test is running on SAI Switches,
  // it should throw an error.
  EXPECT_TRUE(false);
  return 0;
}

} // namespace facebook::fboss::utility
