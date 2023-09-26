/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/sai/hw_test/SaiLinkStateToggler.h"

#include "fboss/agent/hw/sai/switch/SaiPortManager.h"
#include "fboss/agent/hw/sai/switch/SaiSwitch.h"
#include "fboss/agent/test/TestEnsembleIf.h"

namespace facebook::fboss {

SaiSwitch* SaiLinkStateToggler::getHw() const {
  return static_cast<SaiSwitch*>(getHwSwitchEnsemble()->getHwSwitch());
}

std::unique_ptr<HwLinkStateToggler> createHwLinkStateToggler(
    TestEnsembleIf* ensemble) {
  return std::make_unique<SaiLinkStateToggler>(ensemble);
}
} // namespace facebook::fboss
