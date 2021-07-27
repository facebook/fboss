// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/qsfp_service/if/gen-cpp2/qsfp_service_config_types.h"
#include "fboss/qsfp_service/if/gen-cpp2/transceiver_types.h"
#include "fboss/qsfp_service/lib/QsfpConfigParserHelper.h"

using namespace facebook::fboss;

TEST(QsfpConfigParserTest, matchFound) {
  cfg::TransceiverConfigOverrideFactor factor;
  cfg::TransceiverConfigOverrideFactor moduleFactor;
  moduleFactor.applicationCode_ref() = SMFMediaInterfaceCode::CWDM4_100G;
  // None of the override factors specified. This should return a match found
  EXPECT_TRUE(overrideFactorMatchFound(factor, moduleFactor));
  // Specify an application code in the configured factor
  factor.applicationCode_ref() = SMFMediaInterfaceCode::CWDM4_100G;
  // Check for exact match
  EXPECT_TRUE(overrideFactorMatchFound(factor, moduleFactor));
  // Check when there is not a match
  moduleFactor.applicationCode_ref() = SMFMediaInterfaceCode::FR4_200G;
  EXPECT_FALSE(overrideFactorMatchFound(factor, moduleFactor));
  // Check when an optional field is not set in the module's factor
  moduleFactor.applicationCode_ref().reset();
  EXPECT_FALSE(overrideFactorMatchFound(factor, moduleFactor));
}

TEST(QsfpConfigParserTest, rxEqSettingOverride) {
  cfg::TransceiverOverrides cfgOverride;
  cfg::CmisOverrides cmisOverride;

  cfgOverride.cmis_ref() = cmisOverride;
  EXPECT_EQ(cmisRxEqualizerSettingOverride(cfgOverride), std::nullopt);

  RxEqualizerSettings rxEq;
  rxEq.mainAmplitude_ref() = 2;
  cmisOverride.rxEqualizerSettings_ref() = rxEq;
  cfgOverride.cmis_ref() = cmisOverride;

  EXPECT_EQ(cmisRxEqualizerSettingOverride(cfgOverride), rxEq);
}

TEST(QsfpConfigParserTest, preemphOverride) {
  cfg::TransceiverOverrides cfgOverride;
  cfg::Sff8636Overrides sffOverride;

  cfgOverride.sff_ref() = sffOverride;
  EXPECT_EQ(sffRxPreemphasisOverride(cfgOverride), std::nullopt);

  sffOverride.rxPreemphasis_ref() = 2;
  cfgOverride.sff_ref() = sffOverride;
  EXPECT_EQ(sffRxPreemphasisOverride(cfgOverride), 2);
}
