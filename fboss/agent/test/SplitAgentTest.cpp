// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/SplitAgentTest.h"
#include "fboss/agent/HwAsicTable.h"

DEFINE_bool(run_forever, false, "run the test forever");
DEFINE_bool(run_forever_on_failure, false, "run the test forever on failure");

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
  if (FLAGS_run_forever ||
      (::testing::Test::HasFailure() && FLAGS_run_forever_on_failure)) {
    runForever();
  }
  tearDownAgentEnsemble();
  /*
   * Work around to avoid long singleton cleanup
   * during atexit. TODO: figure out why extra time
   * is spent in at exit cleanups
   */
  XLOG(DBG2) << " Destroy singleton instances ";
  folly::SingletonVault::singleton()->destroyInstances();
  XLOG(DBG2) << " Done destroying singleton instances";
}

void SplitAgentTest::tearDownAgentEnsemble(bool doWarmboot) {
  if (!agentEnsemble_) {
    return;
  }

  if (::testing::Test::HasFailure()) {
    // TODO: Collect Info and dump counters
  }
  if (doWarmboot) {
    agentEnsemble_->gracefulExit();
  }
  agentEnsemble_.reset();
}

void SplitAgentTest::runForever() const {
  XLOG(DBG2) << "SplitAgentTest run forever...";
  while (true) {
    sleep(1);
    XLOG_EVERY_MS(DBG2, 5000) << "SplitAgentTest running forever";
  }
}

const std::map<SwitchID, HwAsic*> SplitAgentTest::getAsics() const {
  return agentEnsemble_->getSw()->getHwAsicTable()->getHwAsics();
}

bool SplitAgentTest::isSupportedOnAllAsics(HwAsic::Feature feature) const {
  // All Asics supporting the feature
  return agentEnsemble_->getSw()->getHwAsicTable()->isFeatureSupportedOnAllAsic(
      feature);
}

} // namespace facebook::fboss
