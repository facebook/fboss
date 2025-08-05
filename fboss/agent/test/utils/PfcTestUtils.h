// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <memory>

#include "fboss/agent/TxPacket.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/agent/hw/gen-cpp2/hardware_stats_types.h"
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/TestEnsembleIf.h"

namespace facebook::fboss::utility {

struct PfcBufferParams {
  static constexpr auto kSmallGlobalSharedBytes{20000};
  // TODO(maxgg): Change this back 85344 once CS00012382848 is fixed.
  static constexpr auto kGlobalSharedBytes{1500000};
  static constexpr auto kGlobalHeadroomBytes{5000}; // keep this small

  int globalShared{0};
  int globalHeadroom{0};
  int minLimit{0};
  int pgHeadroom{0};
  facebook::fboss::cfg::MMUScalingFactor scalingFactor;
  std::optional<int> resumeOffset;
  std::optional<int> resumeThreshold;
  std::optional<int> pgShared;

  static PfcBufferParams getPfcBufferParams(
      cfg::AsicType asicType,
      int globalShared = kGlobalSharedBytes,
      int globalHeadroom = kGlobalHeadroomBytes);
};

void setupPfcBuffers(
    const TestEnsembleIf* ensemble,
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports,
    const std::vector<int>& losslessPgIds,
    const std::vector<int>& lossyPgIds,
    const std::map<int, int>& tcToPgOverride = {});

void setupPfcBuffers(
    const TestEnsembleIf* ensemble,
    cfg::SwitchConfig& cfg,
    const std::vector<PortID>& ports,
    const std::vector<int>& losslessPgIds,
    const std::vector<int>& lossyPgIds,
    const std::map<int, int>& tcToPgOverride,
    PfcBufferParams buffer);

void addPuntPfcPacketAcl(cfg::SwitchConfig& cfg, uint16_t queueId);

std::string pfcStatsString(const HwPortStats& stats);

std::unique_ptr<TxPacket> makePfcFramePacket(
    const AgentEnsemble& ensemble,
    uint8_t classVector);

} // namespace facebook::fboss::utility
