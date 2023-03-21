/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include <glog/logging.h>
#include <gtest/gtest.h>

#include "fboss/agent/FbossError.h"
#include "fboss/agent/hw/bcm/BcmCosQueueFBConvertors.h"
#include "fboss/agent/hw/bcm/tests/BcmUnitTestUtils.h"

// Need to define bde in a single cpp_unittest
extern "C" {
struct ibde_t;
ibde_t* bde;
}

namespace {
using namespace facebook::fboss;
cfg::ActiveQueueManagement getEarlyDropAqmConfig() {
  cfg::ActiveQueueManagement earlyDropAQM;
  cfg::LinearQueueCongestionDetection earlyDropLQCD;
  *earlyDropLQCD.minimumLength() = 208;
  *earlyDropLQCD.maximumLength() = 416;
  earlyDropLQCD.probability() = 50;
  earlyDropAQM.detection()->linear_ref() = earlyDropLQCD;
  *earlyDropAQM.behavior() = cfg::QueueCongestionBehavior::EARLY_DROP;
  return earlyDropAQM;
}

cfg::ActiveQueueManagement getECNAqmConfig() {
  cfg::ActiveQueueManagement ecnAQM;
  cfg::LinearQueueCongestionDetection ecnLQCD;
  *ecnLQCD.minimumLength() = 624;
  *ecnLQCD.maximumLength() = 624;
  ecnLQCD.probability() = 100;
  ecnAQM.detection()->linear_ref() = ecnLQCD;
  *ecnAQM.behavior() = cfg::QueueCongestionBehavior::ECN;
  return ecnAQM;
}
} // unnamed namespace

namespace facebook::fboss {

using namespace facebook::fboss::utility;

TEST(CosQueueBcmConvertors, cfgAlphaToFromBcm) {
  for (auto cfgAlphaAndName : cfg::_MMUScalingFactor_VALUES_TO_NAMES) {
    auto cfgAlpha = cfgAlphaAndName.first;
    if (cfgAlpha == cfg::MMUScalingFactor::ONE_32768TH) {
      // Unsupported on XGS
      continue;
    }
    auto bcmAlpha = cfgAlphaToBcmAlpha(cfgAlpha);
    EXPECT_EQ(cfgAlpha, bcmAlphaToCfgAlpha(bcmAlpha));
  }
  EXPECT_THROW(
      bcmAlphaToCfgAlpha(bcmCosqControlDropLimitAlphaCount), FbossError);
}

TEST(CosQueueBcmConvertors, cfgAqmToFromBcm) {
  std::array<cfg::ActiveQueueManagement, 2> aqms = {
      getEarlyDropAqmConfig(), getECNAqmConfig()};

  facebook::fboss::cfg::LinearQueueCongestionDetection detection;
  *detection.minimumLength() = *detection.maximumLength() = 100000;
  for (const auto& aqm : aqms) {
    // w/ threshold
    auto defaultAqm = aqm;
    defaultAqm.detection()->linear_ref() = detection;
    bcm_cosq_gport_discard_t discard =
        cfgAqmToBcmAqm(*aqm.behavior(), *aqm.detection(), defaultAqm);
    auto cfgAqmOpt = bcmAqmToCfgAqm(discard, defaultAqm);
    EXPECT_TRUE(cfgAqmOpt.has_value());
    EXPECT_EQ(aqm, cfgAqmOpt.value());

    // w/o threshold
    discard = cfgAqmToBcmAqm(*aqm.behavior(), std::nullopt, defaultAqm);
    cfgAqmOpt = bcmAqmToCfgAqm(discard, defaultAqm);
    EXPECT_FALSE(cfgAqmOpt.has_value());
  }
}

TEST(CosQueueBcmConvertors, cfgSchedulingAndWeightToFromBcm) {
  std::array<std::pair<cfg::QueueScheduling, int>, 2> schedulingAndWeights = {
      std::make_pair(cfg::QueueScheduling::STRICT_PRIORITY, 1),
      std::make_pair(cfg::QueueScheduling::WEIGHTED_ROUND_ROBIN, 9),
  };
  for (const auto& schedulingAndWeight : schedulingAndWeights) {
    auto bcmPair = cfgSchedulingAndWeightToBcm(schedulingAndWeight);
    auto cfgPair = bcmSchedulingAndWeightToCfg(bcmPair);
    EXPECT_EQ(schedulingAndWeight.first, cfgPair.first);
    EXPECT_EQ(schedulingAndWeight.second, cfgPair.second);
  }

  EXPECT_THROW(
      bcmSchedulingAndWeightToCfg(std::make_pair(BCM_COSQ_ROUND_ROBIN, 0)),
      FbossError);
}

} // namespace facebook::fboss
