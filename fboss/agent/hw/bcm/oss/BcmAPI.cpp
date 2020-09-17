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

extern "C" {
#include <systems/bde/linux/linux-bde.h>
}

extern "C" {
/*
 * Define the bde global variable.
 * This is declared in <ibde.h>, but needs to be defined by our code.
 */
ibde_t* bde;
}

namespace facebook::fboss {

/*
 * Initialize the Broadcom SDK and create the BcmAPI singleton.
 *
 * This must be called before using any other Broadcom SDK functions.
 */
void BcmAPI::initImpl() {}
void BcmAPI::bdeCreateSim() {}
void BcmAPI::initHSDKImpl(const std::string& /* yamlConfig */) {}
bool BcmAPI::isHwUsingHSDK() {
  return false;
}

/*
 * Get the number of Broadcom switching devices in this system.
 */
size_t BcmAPI::getNumSwitches() {
  return bde->num_devices(BDE_SWITCH_DEVICES);
}

void BcmAPI::shutdown() {}
} // namespace facebook::fboss
