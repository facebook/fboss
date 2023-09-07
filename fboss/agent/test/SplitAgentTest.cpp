// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/agent/test/SplitAgentTest.h"
#include "fboss/agent/HwAsicTable.h"

DEFINE_bool(run_forever, false, "run the test forever");
DEFINE_bool(run_forever_on_failure, false, "run the test forever on failure");

namespace facebook::fboss {
void SplitAgentTest::SetUp() {
  FLAGS_verify_apply_oper_delta = true;
  FLAGS_hide_fabric_ports = hideFabricPorts();
  // Reset any global state being tracked in singletons
  // Each test then sets up its own state as needed.
  folly::SingletonVault::singleton()->destroyInstances();
  folly::SingletonVault::singleton()->reenableInstances();
  // Set watermark stats update interval to 0 so we always refresh BST stats
  // in each updateStats call
  FLAGS_update_watermark_stats_interval_s = 0;

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

AgentEnsemble* SplitAgentTest::getAgentEnsemble() const {
  return agentEnsemble_.get();
}

const std::shared_ptr<SwitchState> SplitAgentTest::getProgrammedState() const {
  return getAgentEnsemble()->getProgrammedState();
}

bool SplitAgentTest::hideFabricPorts() const {
  // Due to the speedup in test run time (6m->21s on meru400biu)
  // we want to skip over fabric ports in a overwhelming
  // majority of test cases. Make this the default HwTest mode
  return true;
}

} // namespace facebook::fboss
