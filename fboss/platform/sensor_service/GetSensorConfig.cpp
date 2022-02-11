// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/platform/sensor_service/GetSensorConfig.h"

#include "fboss/agent/FbossError.h"
#include "fboss/lib/platforms/PlatformMode.h"
#include "fboss/lib/platforms/PlatformProductInfo.h"

namespace facebook::fboss::platform::sensor_service {

std::string getDarwinConfig();
std::string getMockConfig();

std::string getPlatformConfig() {
  fboss::PlatformProductInfo productInfo(FLAGS_fruid_filepath);
  productInfo.initialize();
  switch (productInfo.getMode()) {
    case PlatformMode::DARWIN:
      return getDarwinConfig();
      break;
    case PlatformMode::FAKE_WEDGE:
      return getMockConfig();
      break;
    default:
      throw FbossError(
          "Unhandled platform : ", toString(productInfo.getMode()));
  }
  return "";
}
} // namespace facebook::fboss::platform::sensor_service
