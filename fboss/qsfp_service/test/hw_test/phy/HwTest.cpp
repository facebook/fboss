/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/qsfp_service/test/hw_test/phy/HwTest.h"

#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsemble.h"
#include "fboss/qsfp_service/test/hw_test/phy/HwPhyEnsembleFactory.h"

namespace facebook::fboss {
void HwTest::SetUp() {
  // TODO(joseph5wu) Prepare HwPhyEnsemble::HwPhyEnsembleInfo to pick up just
  // one pim to initialize
  HwPhyEnsemble::HwPhyEnsembleInitInfo initInfo;
  ensemble_ = createHwEnsemble(initInfo);
}

void HwTest::TearDown() {
  // TODO(joseph5wu) Add entra step before reset the ensemble
  ensemble_.reset();
}
} // namespace facebook::fboss
