// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestSwitchUtils.h"

#include "fboss/agent/hw/HwSwitchStats.h"
#include "fboss/agent/hw/test/HwSwitchEnsemble.h"

#include <gtest/gtest.h>

namespace facebook::fboss::utility {
void generateParityError(HwSwitchEnsemble* ensemble) {
  auto stats = ensemble->getHwSwitch()->getSwitchStats();
  EXPECT_EQ(stats->getCorrParityErrorCount(), 0);
  std::string out;
  ensemble->runDiagCommand(
      "ser INJECT memory=L2_ENTRY index=10 pipe=pipe_x\n", out);
  ensemble->runDiagCommand("d chg L2_ENTRY 10 1\n", out);
  std::this_thread::sleep_for(std::chrono::milliseconds(500));
  std::ignore = out;
}

void verifyParityError(HwSwitchEnsemble* ensemble) {
  auto stats = ensemble->getHwSwitch()->getSwitchStats();
  EXPECT_GT(stats->getCorrParityErrorCount(), 0);
}
} // namespace facebook::fboss::utility
