/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/benchmarks/BcmBenchmarker.h"

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

std::unique_ptr<BcmBenchmarker> setupBenchmarker() {
  BcmAPI::init(BcmConfig::loadFromFile(FLAGS_bcm_config));
  auto benchmarker = std::make_unique<BcmBenchmarker>();
  benchmarker->init();
  return benchmarker;
}

std::unique_ptr<HwSwitch> BcmBenchmarker::createHwSwitch(Platform* platform) {
  return std::make_unique<BcmSwitch>(static_cast<BcmPlatform*>(platform));
}

} // namespace fboss
} // namespace facebook
