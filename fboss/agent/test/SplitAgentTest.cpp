// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/SplitAgentTest.h"

namespace facebook::fboss {
void SplitAgentTest::SetUp() {
  if (initialConfigFn_ && platformConfigFn_) {
    agentEnsemble_ = createAgentEnsemble(initialConfigFn_, platformConfigFn_);
  } else if (initialConfigFn_) {
    agentEnsemble_ = createAgentEnsemble(initialConfigFn_);
  } else {
    throw FbossError(
        "InitialConfigFn needs to be set during SplitAgentTest SetUp()");
  }
}
void SplitAgentTest::TearDown() {
  // TODO: Deinit
}
} // namespace facebook::fboss
