// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/PmUnitInfoFetcher.h"

namespace facebook::fboss::platform::sensor_service {
std::optional<std::array<int16_t, 3>> PmUnitInfoFetcher::fetch(
    const std::string& /* slotPath */,
    const std::string& /* pmUnitName */) const {
  return std::nullopt;
}
} // namespace facebook::fboss::platform::sensor_service
