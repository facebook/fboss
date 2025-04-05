#include "fboss/agent/test/utils/AgentFsdbIntegrationBenchmarkHelper.h"
#include "fboss/agent/AgentFsdbSyncManager.h"
#include "fboss/fsdb/if/FsdbModel.h"

DEFINE_bool(
    fsdb_publish_test,
    false,
    "Test-flag indicating whether to publish to FSDB and wait for completion");

const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;

namespace facebook::fboss {

AgentFsdbIntegrationBenchmarkHelper::AgentFsdbIntegrationBenchmarkHelper() {
  fsdb::FsdbStateSubscriber::FsdbOperStateUpdateCb subscriptionCb =
      [&](fsdb::OperState&& /*operState*/) {};
  if (FLAGS_fsdb_publish_test) {
    enablePublishToFsdb_ = true;
    connectToFsdb_ = true;
    waitForPublishConfirmed_ = true;
  }

  if (waitForPublishConfirmed_) {
    // create a test subscription for agent state to help in confirming
    // completion of publish to fsdb
    auto stateChangeCb = [this](
                             fsdb::SubscriptionState /*oldState*/,
                             fsdb::SubscriptionState newState,
                             std::optional<bool> /*initialSyncHasData*/) {
      if (newState == fsdb::SubscriptionState::CONNECTED) {
        subscriptionConnected_.post();
      }
    };

    pubsubMgr_ = std::make_unique<fsdb::FsdbPubSubManager>("bench_client");
    pubsubMgr_->addStatePathSubscription(
        stateRoot.agent().switchState().portMaps().tokens(),
        std::move(stateChangeCb),
        std::move(subscriptionCb));
  }
}

void AgentFsdbIntegrationBenchmarkHelper::awaitCompletion(
    AgentEnsemble* /* ensemble */) {
  if (waitForPublishConfirmed_) {
    // wait for subscribption to be served
    subscriptionConnected_.wait();
  }
}

AgentFsdbIntegrationBenchmarkHelper::~AgentFsdbIntegrationBenchmarkHelper() {
  if (pubsubMgr_) {
    pubsubMgr_.reset();
  }
}

} // namespace facebook::fboss
