// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <algorithm>

#include <re2/re2.h>

#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_config_types.h"

namespace facebook::fboss::platform::sensor_service {

// Returns true if the slot is a field-replaceable PSU or PEM. FullMatch
// requires the entire name to be PSU<digits> or PEM<digits>, so decorated
// names like PSU_BRICK, PSU_PWRBRK, PEM_OUT — and unrelated slots like
// HSC, PWRBRK — are all rejected.
inline bool isPsuOrPem(const sensor_config::PerSlotPowerConfig& slot) {
  static const re2::RE2 kPsuPemPattern("(PSU|PEM)\\d+");
  return re2::RE2::FullMatch(*slot.name(), kPsuPemPattern);
}

// Returns true if any perSlotPowerConfig matches a field-replaceable PSU
// or PEM (delegates to isPsuOrPem above for the exact matching rules).
inline bool hasPsuOrPem(const sensor_config::PowerConfig& powerConfig) {
  return std::any_of(
      powerConfig.perSlotPowerConfigs()->begin(),
      powerConfig.perSlotPowerConfigs()->end(),
      isPsuOrPem);
}

} // namespace facebook::fboss::platform::sensor_service
