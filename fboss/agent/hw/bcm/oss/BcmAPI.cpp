/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmAPI.h"
#include "fboss/agent/hw/bcm/BcmUnit.h"
#include "fboss/agent/hw/bcm/BcmWarmBootHelper.h"
#include <folly/Memory.h>

#include <stdlib.h>

namespace facebook { namespace fboss {

#define NUM_UNITS 1

std::atomic<BcmUnit*> bcmUnits[NUM_UNITS];

/*
 * Initialize the Broadcom SDK and create the BcmAPI singleton.
 *
 * This must be called before using any other Broadcom SDK functions.
 */
void BcmAPI::initImpl(const std::map<std::string, std::string>& /*config*/) {}

/*
 * Get the number of Broadcom switching devices in this system.
 */
size_t BcmAPI::getNumSwitches() {
  return NUM_UNITS;
}

/*
 * Get the number of Broadcom switching devices in this system.
 */
size_t BcmAPI::getMaxSwitches() {
  return NUM_UNITS;
}

/*
 * Get the thread name defined for this thread by the Broadcom SDK.
 */
std::string BcmAPI::getThreadName() {
  return "";
}

/*
 * Get hw config
 */
BcmAPI::HwConfigMap BcmAPI::getHwConfig() {
  return HwConfigMap();
}
}}
