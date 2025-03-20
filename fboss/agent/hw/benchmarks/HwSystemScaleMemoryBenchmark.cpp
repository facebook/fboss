#include <folly/synchronization/Baton.h>
#include "fboss/agent/test/AgentEnsemble.h"
#include "fboss/agent/test/utils/SystemScaleTestUtils.h"
#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/if/facebook/gen-cpp2-thriftpath/fsdb_model.h" // @manual=//fboss/fsdb/if/facebook:fsdb_model-cpp2-thriftpath
#include "fboss/fsdb/if/facebook/gen-cpp2/fsdb_model_types.h"
#include "gflags/gflags.h"

#include <folly/Benchmark.h>

DEFINE_bool(
    fsdb_publish_test,
    false,
    "Test-flag indicating whether to publish to FSDB and wait for completion");

const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;

namespace facebook::fboss {

BENCHMARK(SystemScaleMemoryBenchmark) {
  auto ensemble = createAgentEnsemble(
      utility::getSystemScaleTestSwitchConfiguration,
      false /*disableLinkStateToggler*/);

  // if publishing to fsdb, create a test subscription for agent
  // state to help in confirming completion of publish to fsdb
  std::unique_ptr<fsdb::FsdbPubSubManager> pubsubMgr;
  folly::Baton<> subscriptionConnected;
  if (FLAGS_fsdb_publish_test) {
    fsdb::FsdbStateSubscriber::FsdbOperStateUpdateCb subscriptionCb =
        [&](fsdb::OperState&& /*operState*/) {};
    auto stateChangeCb = [&subscriptionConnected](
                             fsdb::SubscriptionState /*oldState*/,
                             fsdb::SubscriptionState newState,
                             std::optional<bool> /*initialSyncHasData*/) {
      if (newState == fsdb::SubscriptionState::CONNECTED) {
        subscriptionConnected.post();
      }
    };

    pubsubMgr = std::make_unique<fsdb::FsdbPubSubManager>("bench_client");
    pubsubMgr->addStatePathSubscription(
        stateRoot.agent().switchState().portMaps().tokens(),
        stateChangeCb,
        subscriptionCb);
  }

  // benchmark test: max scale agent state
  utility::initSystemScaleTest(ensemble.get());

  // if publishing to fsdb, wait for subscribption to be served
  if (FLAGS_fsdb_publish_test) {
    subscriptionConnected.wait();

    pubsubMgr.reset();
  }
}
} // namespace facebook::fboss
