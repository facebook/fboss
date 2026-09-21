// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/platforms/common/PlatformMapping.h"

namespace facebook::fboss {

bool PlatformPortProfileConfigMatcher::isTransceiverVendorOverrideMatch(
    const std::string& /*overrideVendorName*/,
    const std::string& /*overridePartNum*/,
    const std::string& /*currentVendorName*/,
    const std::string& /*currentPartNum*/) const {
  return false;
}

} // namespace facebook::fboss
