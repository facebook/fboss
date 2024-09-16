// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include <gtest/gtest.h>
#include "fboss/agent/test/link_tests/AgentEnsembleLinkTest.h"
#include "fboss/agent/test/link_tests/LinkTestUtils.h"

using namespace ::testing;
using namespace facebook::fboss;

class AgentEnsembleEmptyLinkTest : public AgentEnsembleLinkTest {};

TEST_F(AgentEnsembleEmptyLinkTest, CheckInit) {
  verifyAcrossWarmBoots(
      []() {},
      [this]() {
        utility::getAllTransceiverConfigValidationStatuses(
            getCabledTranceivers());
      });
}
