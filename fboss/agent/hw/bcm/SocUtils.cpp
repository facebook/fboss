// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/hw/bcm/SocUtils.h"

#include "fboss/agent/hw/bcm/BcmError.h"

extern "C" {
#if (!defined(BCM_VER_MAJOR))
#include <soc/opensoc.h>
#else
#include <soc/cm.h>
#include <soc/devids.h>
#endif
}

namespace facebook::fboss {

uint16_t SocUtils::getDeviceId(int unit) {
  uint16_t devId;
  uint8_t revId;

  auto rv = soc_cm_get_id(unit, &devId, &revId);
  bcmCheckError(rv, "Unable to get device id");

  return devId;
}

bool SocUtils::isTrident2(int unit) {
  return getDeviceId(unit) == BCM56850_DEVICE_ID;
}

bool SocUtils::isTomahawk(int unit) {
  return getDeviceId(unit) == BCM56960_DEVICE_ID;
}

bool SocUtils::isTomahawk3(int unit) {
  return getDeviceId(unit) == BCM56980_DEVICE_ID;
}

} // namespace facebook::fboss
