// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/agent/gen-cpp2/switch_config_types.h"
#include "fboss/lib/phy/gen-cpp2/phy_types.h"

namespace facebook::fboss::utility {

bool isReedSolomonFec(phy::FecMode fec);

double
ber(uint64_t erroredBits, cfg::PortSpeed speed, uint64_t timeDeltaInSeconds);

void updateCorrectedBitsAndPreFECBer(
    phy::RsFecInfo& fecInfo,
    const phy::RsFecInfo& oldRsFecInfo,
    std::optional<uint64_t> correctedBitsFromHw,
    int timeDeltaInSeconds,
    phy::FecMode fecMode,
    cfg::PortSpeed speed);

void updateSignalDetectChangedCount(
    int changedCount,
    int lane,
    phy::LaneInfo& curr,
    phy::PmdInfo& prev);
void updateCdrLockChangedCount(
    int changedCount,
    int lane,
    phy::LaneInfo& curr,
    phy::PmdInfo& prev);

void updateSignalDetectChangedCount(
    int changedCount,
    int lane,
    phy::LaneStats& curr,
    phy::PmdStats& prev);
void updateCdrLockChangedCount(
    int changedCount,
    int lane,
    phy::LaneStats& curr,
    phy::PmdStats& prev);
} // namespace facebook::fboss::utility
