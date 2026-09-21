// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/test/link_tests/LinkTest.h"
#include "fboss/agent/test/link_tests/LinkTestUtils.h"

using namespace ::testing;
using namespace facebook::fboss;

class EmptyLinkTest : public LinkTest {
  std::vector<link_test_production_features::LinkTestProductionFeature>
  getProductionFeatures() const override {
    return {
        link_test_production_features::LinkTestProductionFeature::L1_LINK_TEST,
        link_test_production_features::LinkTestProductionFeature::L2_LINK_TEST};
  }
};

TEST_F(EmptyLinkTest, CheckInit) {
  verifyAcrossWarmBoots(
      []() {},
      [this]() {
        utility::getAllTransceiverConfigValidationStatuses(
            getCabledTranceivers());
      });
}
