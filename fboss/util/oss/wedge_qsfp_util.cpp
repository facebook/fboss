// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/util/wedge_qsfp_util.h"
#include <sysexits.h>
#include <memory>
#include <utility>
#include "fboss/lib/if/gen-cpp2/fboss_common_types.h"
#include "fboss/lib/usb/TransceiverI2CApi.h"
#include "fboss/lib/usb/TransceiverPlatformApi.h"

namespace facebook::fboss {

std::pair<std::unique_ptr<TransceiverI2CApi>, int>
getTransceiverIOBusFromPlatform(const std::string& /* platform */) {
  return std::make_pair(nullptr, EX_USAGE);
}

std::pair<std::unique_ptr<TransceiverI2CApi>, int> getTransceiverIOBusFromMode(
    PlatformType /* platformType */) {
  return std::make_pair(nullptr, EX_USAGE);
}

std::pair<std::unique_ptr<TransceiverPlatformApi>, int>
getTransceiverPlatformAPIFromMode(
    PlatformType /* platformType */,
    TransceiverI2CApi* /* i2cBus */) {
  return std::make_pair(nullptr, EX_USAGE);
}

} // namespace facebook::fboss
