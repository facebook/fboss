/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/tests/BcmSwitchEnsemble.h"

#include "fboss/agent/AlpmUtils.h"
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmConfig.h"
#include "fboss/agent/hw/bcm/BcmPlatform.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/state/SwitchState.h"

#include <memory>

DECLARE_string(bcm_config);

namespace facebook {
namespace fboss {

BcmSwitchEnsemble::BcmSwitchEnsemble() {
  BcmAPI::init(BcmConfig::loadFromFile(FLAGS_bcm_config));
}

std::unique_ptr<HwSwitch> BcmSwitchEnsemble::createHwSwitch(
    Platform* platform,
    uint32_t featuresDesired) {
  return std::make_unique<BcmSwitch>(
      static_cast<BcmPlatform*>(platform), featuresDesired);
}

} // namespace fboss
} // namespace facebook
