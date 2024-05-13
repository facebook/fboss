// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/test/HwTestPfcUtils.h"
#include <gtest/gtest.h>

namespace facebook::fboss::utility {

void checkPfcWdSwHwCfgMatch(
    const facebook::fboss::cfg::PfcWatchdog& swCfg,
    const facebook::fboss::cfg::PfcWatchdog& hwCfg) {
  EXPECT_EQ(*swCfg.detectionTimeMsecs(), *hwCfg.detectionTimeMsecs());
  EXPECT_EQ(*swCfg.recoveryTimeMsecs(), *hwCfg.recoveryTimeMsecs());
  EXPECT_EQ(*swCfg.recoveryAction(), *hwCfg.recoveryAction());
  EXPECT_EQ(swCfg, hwCfg);
}

} // namespace facebook::fboss::utility
