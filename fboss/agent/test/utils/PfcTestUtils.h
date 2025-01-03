// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/Utils.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/test/AgentEnsemble.h"

namespace facebook::fboss::utility {

struct PfcBufferParams {
  // TODO(maxgg): Change this back to 20000 once CS00012382848 is fixed.
  static constexpr auto kGlobalSharedBytes{1000000};
  static constexpr auto kGlobalHeadroomBytes{
      5000}; // keep this lower than globalShared

  int globalShared = kGlobalSharedBytes;
  int globalHeadroom = kGlobalHeadroomBytes;
  int pgLimit = 2200;
  int pgHeadroom = 2200; // keep this lower than globalShared
  std::optional<facebook::fboss::cfg::MMUScalingFactor> scalingFactor;
  int resumeOffset = 1800;
};

void setupPfcBuffers(
    facebook::fboss::AgentEnsemble* ensemble,
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports,
    const std::vector<int>& losslessPgIds,
    const std::map<int, int>& tcToPgOverride = {},
    PfcBufferParams buffer = PfcBufferParams{});

void addPuntPfcPacketAcl(cfg::SwitchConfig& cfg, uint16_t queueId);

std::string pfcStatsString(const HwPortStats& stats);

} // namespace facebook::fboss::utility
