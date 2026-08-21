// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/phy/PhyUtils.h"
#include <fboss/lib/phy/gen-cpp2/phy_types.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include "fboss/agent/gen-cpp2/switch_config_types.h"

namespace facebook::fboss::utility {

namespace {
constexpr auto kMaxCorrectedBits =
    static_cast<uint64_t>(std::numeric_limits<int64_t>::max());

// correctedBits is an i64, so a larger count would wrap negative and yield a
// negative BER
int64_t saturateCorrectedBits(uint64_t correctedBits) {
  return static_cast<int64_t>(std::min(correctedBits, kMaxCorrectedBits));
}

// Saturates before multiplying, since the product itself can overflow
int64_t saturateCorrectedBits(uint64_t count, uint64_t bitsPerCount) {
  return count > kMaxCorrectedBits / bitsPerCount
      ? saturateCorrectedBits(kMaxCorrectedBits)
      : saturateCorrectedBits(count * bitsPerCount);
}
} // namespace

bool isReedSolomonFec(phy::FecMode fec) {
  switch (fec) {
    case phy::FecMode::CL91:
    case phy::FecMode::RS528:
    case phy::FecMode::RS544:
    case phy::FecMode::RS544_2N:
    case phy::FecMode::RS545:
      return true;
    case phy::FecMode::CL74:
    case phy::FecMode::NONE:
      return false;
  }
  return false;
}

uint8_t reedSolomonFecLanes(cfg::PortSpeed speed) {
  switch (speed) {
    case cfg::PortSpeed::TWENTYFIVEG:
      return 1;
    case cfg::PortSpeed::FIFTYG:
    case cfg::PortSpeed::FIFTYTHREEPOINTONETWOFIVEG:
      return 2;
    case cfg::PortSpeed::HUNDREDG:
      return 4;
    case cfg::PortSpeed::TWOHUNDREDG:
      return 8;
    case cfg::PortSpeed::FOURHUNDREDG:
      return 16;
    default:
      return 0;
  }
}

double
ber(uint64_t erroredBits, cfg::PortSpeed speed, uint64_t timeDeltaInSeconds) {
  double totalBits =
      (static_cast<int64_t>(speed) * 1'000'000ul * timeDeltaInSeconds);
  if (totalBits <= 0) {
    return -1;
  }
  return erroredBits / totalBits;
}

// This helper updates the correctedBits and preFECBer counters in RsFecInfo.
// Prefer a hardware corrected-bits count, then estimate from a hardware
// corrected-symbols count, and finally estimate from corrected codewords.
void updateCorrectedBitsAndPreFECBer(
    phy::RsFecInfo& fecInfo,
    const phy::RsFecInfo& oldRsFecInfo,
    std::optional<uint64_t> correctedBitsFromHw,
    std::optional<uint64_t> correctedSymbolsFromHw,
    int timeDeltaInSeconds,
    phy::FecMode fecMode,
    cfg::PortSpeed speed) {
  if (correctedSymbolsFromHw.has_value()) {
    fecInfo.correctedSymbols() = *correctedSymbolsFromHw;
  }
  if (correctedBitsFromHw.has_value()) {
    fecInfo.preFECBerSource() = phy::PreFECBerSource::CORRECTED_BITS;
    fecInfo.correctedBits() = saturateCorrectedBits(*correctedBitsFromHw);
  } else if (correctedSymbolsFromHw.has_value()) {
    // RS-FEC symbols contain 10 bits. Estimate that half of the bits in each
    // corrected symbol were in error.
    constexpr uint64_t kBitsPerCorrectedSymbol = 5;
    fecInfo.preFECBerSource() = phy::PreFECBerSource::CORRECTED_SYMBOLS;
    fecInfo.correctedBits() =
        saturateCorrectedBits(*correctedSymbolsFromHw, kBitsPerCorrectedSymbol);
  } else {
    // RS-528 can correct anywhere from 1 to 70 bits (7 symbols * 10 bits per
    // symbol) in a codeword. RS-544 can correct upto 150 bits (15 symbols).
    // If the corrected bits counter is not available from the hardware, we'll
    // use the upper bound of 70 and 150bits to get the average case BER.
    const uint64_t bitsPerCorrectedCodeword =
        (fecMode == phy::FecMode::CL91 || fecMode == phy::FecMode::RS528) ? 35
                                                                          : 75;
    fecInfo.preFECBerSource() = phy::PreFECBerSource::CORRECTED_CODEWORDS;
    fecInfo.correctedBits() = saturateCorrectedBits(
        *fecInfo.correctedCodewords(), bitsPerCorrectedCodeword);
  }
  auto correctedBitsDelta =
      *fecInfo.correctedBits() - *oldRsFecInfo.correctedBits();
  if (correctedBitsDelta < 0) {
    // A cleared source counter, or a switch to a different preFECBerSource
    // between cycles, makes this delta temporarily negative
    correctedBitsDelta = 0;
  }
  fecInfo.preFECBer() =
      utility::ber(correctedBitsDelta, speed, timeDeltaInSeconds);
}

void updateFecTail(
    phy::RsFecInfo& fecInfo,
    const phy::RsFecInfo& oldRsFecInfo,
    phy::FecMode fecMode) {
  if (fecInfo.codewordStats()->empty()) {
    return;
  }

  short fecTail = 0;
  for (const auto& codewordStat : *fecInfo.codewordStats()) {
    long previousCodewords = 0;
    if (oldRsFecInfo.codewordStats()->find(codewordStat.first) !=
        oldRsFecInfo.codewordStats()->end()) {
      previousCodewords = oldRsFecInfo.codewordStats()->at(codewordStat.first);
    }
    if (previousCodewords < codewordStat.second) {
      // If we have more codewords for a given symbol now than the previous
      // codeword, update the fec tail if its more than the previous tail
      fecTail = fecTail > codewordStat.first ? fecTail : codewordStat.first;
    }
  }
  fecInfo.fecTail() = fecTail;

  switch (fecMode) {
    case phy::FecMode::CL91:
    case phy::FecMode::RS528:
      fecInfo.maxSupportedFecTail() = 7;
      break;
    case phy::FecMode::RS544:
    case phy::FecMode::RS544_2N:
    case phy::FecMode::RS545:
      fecInfo.maxSupportedFecTail() = 15;
      break;
    case phy::FecMode::NONE:
    case phy::FecMode::CL74:
      break;
  }
}

void updateSignalDetectChangedCount(
    int changedCount,
    int lane,
    phy::LaneStats& curr,
    phy::PmdStats& prev) {
  auto prevChangedCount = 0;
  auto it = prev.lanes()->find(lane);
  if (it != prev.lanes()->end()) {
    prevChangedCount =
        prev.lanes()[lane].signalDetectChangedCount().value_or(0);
  }
  curr.signalDetectChangedCount() = changedCount + prevChangedCount;
}

void updateCdrLockChangedCount(
    int changedCount,
    int lane,
    phy::LaneStats& curr,
    phy::PmdStats& prev) {
  auto prevChangedCount = 0;
  auto it = prev.lanes()->find(lane);
  if (it != prev.lanes()->end()) {
    prevChangedCount = prev.lanes()[lane].cdrLockChangedCount().value_or(0);
  }
  curr.cdrLockChangedCount() = changedCount + prevChangedCount;
}

} // namespace facebook::fboss::utility
