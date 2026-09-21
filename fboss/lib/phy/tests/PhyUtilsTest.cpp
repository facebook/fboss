// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/lib/phy/PhyUtils.h"

#include <gtest/gtest.h>

#include <limits>

namespace facebook::fboss::utility {
namespace {

constexpr auto kSpeed = cfg::PortSpeed::TWENTYFIVEG;

TEST(PhyUtilsTest, PrefersCorrectedBitsOverCorrectedSymbols) {
  phy::RsFecInfo fecInfo;
  fecInfo.correctedCodewords() = 4;
  phy::RsFecInfo oldFecInfo;
  oldFecInfo.correctedBits() = 100;

  updateCorrectedBitsAndPreFECBer(
      fecInfo, oldFecInfo, 200, 30, 1, phy::FecMode::RS544, kSpeed);

  EXPECT_EQ(*fecInfo.correctedBits(), 200);
  EXPECT_EQ(*fecInfo.correctedSymbols(), 30);
  EXPECT_EQ(*fecInfo.preFECBerSource(), phy::PreFECBerSource::CORRECTED_BITS);
  EXPECT_DOUBLE_EQ(*fecInfo.preFECBer(), 4e-9);
}

TEST(PhyUtilsTest, EstimatesCorrectedBitsFromCorrectedSymbols) {
  phy::RsFecInfo fecInfo;
  fecInfo.correctedCodewords() = 4;
  phy::RsFecInfo oldFecInfo;
  oldFecInfo.correctedBits() = 100;

  updateCorrectedBitsAndPreFECBer(
      fecInfo, oldFecInfo, std::nullopt, 30, 1, phy::FecMode::RS544, kSpeed);

  EXPECT_EQ(*fecInfo.correctedBits(), 150);
  EXPECT_EQ(*fecInfo.correctedSymbols(), 30);
  EXPECT_EQ(
      *fecInfo.preFECBerSource(), phy::PreFECBerSource::CORRECTED_SYMBOLS);
  EXPECT_DOUBLE_EQ(*fecInfo.preFECBer(), 2e-9);
}

TEST(PhyUtilsTest, SaturatesCorrectedBitsEstimatedFromSymbols) {
  constexpr auto kMaxCorrectedBits = std::numeric_limits<int64_t>::max();
  phy::RsFecInfo fecInfo;
  fecInfo.correctedCodewords() = 4;
  phy::RsFecInfo oldFecInfo;
  oldFecInfo.correctedBits() = kMaxCorrectedBits - 5;

  updateCorrectedBitsAndPreFECBer(
      fecInfo,
      oldFecInfo,
      std::nullopt,
      kMaxCorrectedBits,
      1,
      phy::FecMode::RS544,
      kSpeed);

  EXPECT_EQ(*fecInfo.correctedBits(), kMaxCorrectedBits);
  EXPECT_DOUBLE_EQ(*fecInfo.preFECBer(), 2e-10);
}

TEST(PhyUtilsTest, SaturatesCorrectedBitsFromHw) {
  constexpr auto kMaxCorrectedBits = std::numeric_limits<int64_t>::max();
  phy::RsFecInfo fecInfo;
  fecInfo.correctedCodewords() = 4;
  phy::RsFecInfo oldFecInfo;
  oldFecInfo.correctedBits() = kMaxCorrectedBits - 5;

  updateCorrectedBitsAndPreFECBer(
      fecInfo,
      oldFecInfo,
      std::numeric_limits<uint64_t>::max(),
      std::nullopt,
      1,
      phy::FecMode::RS544,
      kSpeed);

  EXPECT_EQ(*fecInfo.correctedBits(), kMaxCorrectedBits);
  EXPECT_DOUBLE_EQ(*fecInfo.preFECBer(), 2e-10);
}

// The codeword estimator inflates correctedBits, so a switch to the exact HW
// counter drops it sharply.
TEST(PhyUtilsTest, ClampsNegativeDeltaWhenBerSourceChanges) {
  phy::RsFecInfo fecInfo;
  fecInfo.correctedCodewords() = 4;
  phy::RsFecInfo oldFecInfo;
  oldFecInfo.correctedBits() = 100'000;

  updateCorrectedBitsAndPreFECBer(
      fecInfo, oldFecInfo, 200, std::nullopt, 1, phy::FecMode::RS544, kSpeed);

  EXPECT_EQ(*fecInfo.correctedBits(), 200);
  EXPECT_EQ(*fecInfo.preFECBerSource(), phy::PreFECBerSource::CORRECTED_BITS);
  EXPECT_DOUBLE_EQ(*fecInfo.preFECBer(), 0);
}

TEST(PhyUtilsTest, SaturatesCorrectedBitsEstimatedFromCodewords) {
  constexpr auto kMaxCorrectedBits = std::numeric_limits<int64_t>::max();
  phy::RsFecInfo fecInfo;
  fecInfo.correctedCodewords() = kMaxCorrectedBits;
  phy::RsFecInfo oldFecInfo;
  oldFecInfo.correctedBits() = kMaxCorrectedBits - 5;

  updateCorrectedBitsAndPreFECBer(
      fecInfo,
      oldFecInfo,
      std::nullopt,
      std::nullopt,
      1,
      phy::FecMode::RS544,
      kSpeed);

  EXPECT_EQ(*fecInfo.correctedBits(), kMaxCorrectedBits);
  EXPECT_DOUBLE_EQ(*fecInfo.preFECBer(), 2e-10);
}

TEST(PhyUtilsTest, EstimatesCorrectedBitsFromCorrectedCodewords) {
  phy::RsFecInfo fecInfo;
  fecInfo.correctedCodewords() = 4;
  phy::RsFecInfo oldFecInfo;
  oldFecInfo.correctedBits() = 150;

  updateCorrectedBitsAndPreFECBer(
      fecInfo,
      oldFecInfo,
      std::nullopt,
      std::nullopt,
      1,
      phy::FecMode::RS544,
      kSpeed);

  EXPECT_EQ(*fecInfo.correctedBits(), 300);
  EXPECT_EQ(
      *fecInfo.preFECBerSource(), phy::PreFECBerSource::CORRECTED_CODEWORDS);
  EXPECT_DOUBLE_EQ(*fecInfo.preFECBer(), 6e-9);
}

} // namespace
} // namespace facebook::fboss::utility
