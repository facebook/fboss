/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/hw/bcm/BcmUnit.h"

#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#include <ibde.h>
#include <soc/cmext.h>
}

namespace facebook {
namespace fboss {
BcmUnit::BcmUnit(int deviceIndex, BcmPlatform* platform)
    : deviceIndex_(deviceIndex), platform_(platform) {
  unit_ = createHwUnit();
}

BcmUnit::~BcmUnit() {
  deleteBcmUnitImpl();
}

void BcmUnit::attachHSDK(bool /*warmBoot*/) {}

int BcmUnit::createHwUnitHelper(uint16_t deviceID, uint16_t revisionID) {
  // Allocate a unit ID
  // TODO: If IS_OPENNSA is defined, need to check for its version also
  // and then call the corresponding API (this needs to be done
  // when we support OpenNSA versions >= 6.5.26)
#if defined(BCM_SDK_VERSION_GTE_6_5_26)
  return soc_cm_device_create(deviceID, revisionID, this, deviceIndex_);
#else
  return soc_cm_device_create(deviceID, revisionID, this);
#endif
}

int BcmUnit::createHwUnit() {
  auto* dev = bde->get_dev(deviceIndex_);

  // Make sure the device is supported.
  int rv = soc_cm_device_supported(dev->device, dev->rev);
  bcmCheckError(rv, "unsupported device ID ", dev->device, ":", dev->rev);

  // Allocate a unit ID
  return createHwUnitHelper(dev->device, dev->rev);
}

std::pair<uint16_t, uint16_t> BcmUnit::createDRDDevice() {
  throw FbossError("createDRDDevice is unsupported");
}

std::pair<uint16_t, uint16_t> BcmUnit::createSimDRDDevice() {
  throw FbossError("createSimDRDDevice is unsupported");
}

int BcmUnit::destroyHwUnit() {
  int rv = soc_cm_device_destroy(unit_);
  bcmCheckError(rv, "failed to destroy device unit ", unit_);
  return rv;
}

void BcmUnit::detachHSDK() {
  throw FbossError("detachHSDK is unsupported");
}
} // namespace fboss
} // namespace facebook
