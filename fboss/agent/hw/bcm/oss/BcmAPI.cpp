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

extern "C" {
#include <sal/driver.h>
} // extern "C"

namespace {
constexpr auto wbEnvVar = "SOC_BOOT_FLAGS";
constexpr auto wbFlag = "0x200000";
}

namespace facebook { namespace fboss {

#define NUM_UNITS 1

std::atomic<BcmUnit*> bcmUnits[NUM_UNITS];

/*
 * Initialize the Broadcom SDK and create the BcmAPI singleton.
 *
 * This must be called before using any other Broadcom SDK functions.
 */
void BcmAPI::initImpl(const std::map<std::string, std::string>& config){
  /*
   * FIXME(aeckert): unsetenv and setenv are not thread safe. This will
   * be called after the thrift thread has already started so this is
   * not safe to do. We need to fix this once broadcom provides a better
   * API for setting the boot flags.
   */
  unsetenv(wbEnvVar);
  if(bcmUnits[0].load()->warmBootHelper()->canWarmBoot()) {
    setenv(wbEnvVar, wbFlag, 1);
  }

  opennsl_driver_init();
}

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
