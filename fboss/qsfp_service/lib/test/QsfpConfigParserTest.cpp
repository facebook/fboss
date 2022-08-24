// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/lib/QsfpConfigParserHelper.h"

using namespace facebook::fboss;

TEST(QsfpConfigParserTest, matchFound) {
  cfg::TransceiverConfigOverrideFactor factor;
  cfg::TransceiverConfigOverrideFactor moduleFactor;
  moduleFactor.applicationCode() = SMFMediaInterfaceCode::CWDM4_100G;
  // None of the override factors specified. This should return a match found
  EXPECT_TRUE(overrideFactorMatchFound(factor, moduleFactor));
  // Specify an application code in the configured factor
  factor.applicationCode() = SMFMediaInterfaceCode::CWDM4_100G;
  // Check for exact match
  EXPECT_TRUE(overrideFactorMatchFound(factor, moduleFactor));
  // Check when there is not a match
  moduleFactor.applicationCode() = SMFMediaInterfaceCode::FR4_200G;
  EXPECT_FALSE(overrideFactorMatchFound(factor, moduleFactor));
  // Check when an optional field is not set in the module's factor
  moduleFactor.applicationCode().reset();
  EXPECT_FALSE(overrideFactorMatchFound(factor, moduleFactor));
}

TEST(QsfpConfigParserTest, rxEqSettingOverride) {
  cfg::TransceiverOverrides cfgOverride;
  cfg::Sff8636Overrides sffOverride;
  cfg::CmisOverrides cmisOverride;

  cfgOverride.sff_ref() = sffOverride;
  EXPECT_EQ(cmisRxEqualizerSettingOverride(cfgOverride), std::nullopt);
  cfgOverride.cmis_ref() = cmisOverride;
  EXPECT_EQ(cmisRxEqualizerSettingOverride(cfgOverride), std::nullopt);

  RxEqualizerSettings rxEq;
  rxEq.mainAmplitude() = 2;
  cmisOverride.rxEqualizerSettings() = rxEq;
  cfgOverride.cmis_ref() = cmisOverride;

  EXPECT_EQ(cmisRxEqualizerSettingOverride(cfgOverride), rxEq);
}

TEST(QsfpConfigParserTest, preemphOverride) {
  cfg::TransceiverOverrides cfgOverride;
  cfg::Sff8636Overrides sffOverride;
  cfg::CmisOverrides cmisOverride;

  cfgOverride.cmis_ref() = cmisOverride;
  EXPECT_EQ(sffRxPreemphasisOverride(cfgOverride), std::nullopt);
  cfgOverride.sff_ref() = sffOverride;
  EXPECT_EQ(sffRxPreemphasisOverride(cfgOverride), std::nullopt);

  sffOverride.rxPreemphasis() = 2;
  cfgOverride.sff_ref() = sffOverride;
  EXPECT_EQ(sffRxPreemphasisOverride(cfgOverride), 2);
  EXPECT_EQ(sffTxEqualizationOverride(cfgOverride), std::nullopt);
  EXPECT_EQ(sffRxAmplitudeOverride(cfgOverride), std::nullopt);

  sffOverride.rxAmplitude() = 1;
  cfgOverride.sff_ref() = sffOverride;
  EXPECT_EQ(sffRxPreemphasisOverride(cfgOverride), 2);
  EXPECT_EQ(sffTxEqualizationOverride(cfgOverride), std::nullopt);
  EXPECT_EQ(sffRxAmplitudeOverride(cfgOverride), 1);

  sffOverride.txEqualization() = 3;
  cfgOverride.sff_ref() = sffOverride;
  EXPECT_EQ(sffRxPreemphasisOverride(cfgOverride), 2);
  EXPECT_EQ(sffTxEqualizationOverride(cfgOverride), 3);
  EXPECT_EQ(sffRxAmplitudeOverride(cfgOverride), 1);
}
