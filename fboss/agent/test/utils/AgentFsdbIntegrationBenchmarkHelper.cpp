#include "fboss/agent/test/utils/AgentFsdbIntegrationBenchmarkHelper.h"
#include "fboss/agent/AgentFsdbSyncManager.h"
#include "fboss/fsdb/if/FsdbModel.h"

DEFINE_bool(
    fsdb_publish_test,
    false,
    "Test-flag indicating whether to publish to FSDB and wait for completion");

DEFINE_bool(
    wait_for_agent_publish_completion,
    false,
    "Test-flag indicating whether to publish to FSDB and wait for completion");

const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;

const auto portId = 1;

namespace facebook::fboss {

AgentFsdbIntegrationBenchmarkHelper::AgentFsdbIntegrationBenchmarkHelper() {
  auto subscriptionPath = stateRoot.agent().switchState().portMaps();

  fsdb::FsdbStateSubscriber::FsdbOperStateUpdateCb subscriptionCb =
      [&](fsdb::OperState&& state) {
        if (waitForDummyDataPublish_) {
          // thriftpath::RootThriftPath<fsdb::FsdbOperStateRoot> rootPath;
          // auto path = rootPath.agent().switchState().portMaps();
          using PortMaps = typename decltype(subscriptionPath)::DataT;
          PortMaps portMaps;
          if (auto contents = state.contents()) {
            portMaps = apache::thrift::BinarySerializer::deserialize<PortMaps>(
                *contents);
            for (const auto& [_switchIdList, portMap] : portMaps) {
              for (const auto& [portId, portInfo] : portMap) {
                if (portInfo.portId().value() == portId) {
                  if (portInfo.portDescription().value() == "Publish Done") {
                    dummyDataPublished_.post();
                  }
                }
              }
            }
          }
        }
      };
  if (FLAGS_fsdb_publish_test) {
    enablePublishToFsdb_ = true;
    connectToFsdb_ = true;
    waitForPublishConfirmed_ = true;
  }

  if (FLAGS_wait_for_agent_publish_completion) {
    waitForDummyDataPublish_ = true;
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
        subscriptionPath.tokens(),
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
  if (waitForDummyDataPublish_) {
    dummyDataPublished_.wait();
  }
}

void AgentFsdbIntegrationBenchmarkHelper::publishCompletionMarker(
    AgentEnsemble* ensemble) {
  if (waitForDummyDataPublish_) {
    ensemble->getSw()->updateStateBlocking(
        "update port desc", [](const auto& state) {
          auto newState = state->clone();
          auto ports = newState->getPorts()->modify(&newState);
          auto port = ports->getNodeIf(portId)->clone();
          port->setDescription("Publish Done");
          HwSwitchMatcher matcher(std::unordered_set<SwitchID>({SwitchID(0)}));
          ports->updateNode(std::move(port), matcher);
          return newState;
        });
  }
}

AgentFsdbIntegrationBenchmarkHelper::~AgentFsdbIntegrationBenchmarkHelper() {
  if (pubsubMgr_) {
    pubsubMgr_.reset();
  }
}

} // namespace facebook::fboss
