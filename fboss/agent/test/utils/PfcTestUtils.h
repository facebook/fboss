// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/types.h"

namespace facebook::fboss::utility {

struct PfcBufferParams {
  int globalShared = 20000;
  int globalHeadroom = 5000; // keep this lower than globalShared
  int pgLimit = 2200;
  int pgHeadroom = 2200; // keep this lower than globalShared
  std::optional<facebook::fboss::cfg::MMUScalingFactor> scalingFactor =
      facebook::fboss::cfg::MMUScalingFactor::ONE_128TH;
  int resumeOffset = 1800;
};

void setupPfcBuffers(
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports,
    const std::vector<int>& losslessPgIds,
    PfcBufferParams buffer = PfcBufferParams{});

} // namespace facebook::fboss::utility
