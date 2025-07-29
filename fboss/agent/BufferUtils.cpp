// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/agent/BufferUtils.h"
#include "fboss/agent/Utils.h"
#include "fboss/agent/state/PortPgConfig.h"

namespace facebook::fboss::utility {

bool isLosslessPg(const PortPgConfig& pgConfig) {
  return pgConfig.getHeadroomLimitBytes().value_or(0) != 0 ||
      FLAGS_allow_zero_headroom_for_lossless_pg;
}

bool isLosslessPg(const state::PortPgFields& pgConfig) {
  return pgConfig.headroomLimitBytes().value_or(0) != 0 ||
      FLAGS_allow_zero_headroom_for_lossless_pg;
}

} // namespace facebook::fboss::utility
