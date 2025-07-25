#include "fboss/agent/test/utils/AgentFsdbIntegrationBenchmarkHelper.h"

#ifndef IS_OSS
#include <servicerouter/client/cpp2/ServiceRouter.h>
#endif
#include "fboss/agent/AgentFsdbSyncManager.h"

#include "fboss/lib/CommonFileUtils.h"
#include "folly/Benchmark.h"

DEFINE_bool(
    fsdb_publish_test,
    false,
    "Test-flag indicating whether to publish to FSDB and wait for completion");

DEFINE_bool(
    wait_for_agent_publish_completion,
    false,
    "Test-flag indicating whether to publish to FSDB and wait for completion");

DEFINE_string(
    write_agent_config_marker_for_fsdb,
    "",
    "Write marker file for FSDB");
DECLARE_bool(json);
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;

const auto kExpectedPortId = 1;
const auto kExpectedDescription = "Publish Done";

namespace facebook::fboss {

bool AgentFsdbIntegrationBenchmarkHelper::queryFsdbCounters(
    std::map<std::string, int64_t>& fb303Counters) {
  bool success{false};
#ifndef IS_OSS
  // Create a thrift client to fsdb service
  auto clientParams =
      facebook::servicerouter::ClientParams()
          .setSingleHost("::1", FLAGS_fsdbPort)
          .setProcessingTimeoutMs(std::chrono::milliseconds(1000));
  auto fsdbClient =
      facebook::servicerouter::cpp2::getClientFactory()
          .getSRClientUnique<
              apache::thrift::Client<facebook::fboss::fsdb::FsdbService>>(
              "fboss.fsdb", clientParams);
  try {
    fsdbClient->sync_getCounters(fb303Counters);
    success = true;
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Failed to get fb303 counters: " << ex.what();
    std::cerr << "Failed to get fb303 counters: " << ex.what() << std::endl;
  }
#endif
  return success;
};

AgentFsdbIntegrationBenchmarkHelper::AgentFsdbIntegrationBenchmarkHelper() {
  auto subscriptionPath = stateRoot.agent().switchState().portMaps();
  using PortMaps = typename decltype(subscriptionPath)::DataT;

  auto isExpectedPublishMarker = [](fsdb::OperState&& state) {
    PortMaps portMaps;
    if (auto contents = state.contents()) {
      portMaps =
          apache::thrift::BinarySerializer::deserialize<PortMaps>(*contents);
      for (const auto& [_switchIdList, portMap] : portMaps) {
        auto it = portMap.find(kExpectedPortId);
        if (it != portMap.end() &&
            (it->second.portDescription().value() == kExpectedDescription)) {
          return true;
        }
      }
    }
    return false;
  };

  fsdb::FsdbStateSubscriber::FsdbOperStateUpdateCb subscriptionCb =
      [&](fsdb::OperState&& state) {
        auto currentTimestamp =
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count();
        auto lastPublishedAt = state.metadata()->lastPublishedAt().value();
        subscribe_latency_ = std::max<uint64_t>(
            subscribe_latency_, currentTimestamp - lastPublishedAt);
        if (waitForAllPublishConfirmed_) {
          if (isExpectedPublishMarker(std::move(state))) {
            dummyDataPublished_.post();
          }
        }
      };

  if (FLAGS_wait_for_agent_publish_completion) {
    FLAGS_fsdb_publish_test = true; // implicit
    writeAllPublishCompleteMarker_ = true;
  }

  if (FLAGS_fsdb_publish_test) {
    enablePublishToFsdb_ = true;
    waitForSubscriptionConnected_ = true;
  }

  if (enablePublishToFsdb_) {
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

void writeAgentConfigMarkerForFsdb() {
  auto filePath =
      folly::to<std::string>(FLAGS_write_agent_config_marker_for_fsdb);

  if (createFile(filePath) < 0) {
    XLOG(DBG2) << "Failed to create file: " << filePath;
    return;
  }

  while (true) {
    if (!checkFileExists(filePath)) {
      XLOG(DBG2) << "FSDB done with benchmarking and has deleted marker file";
      break;
    } else {
      std::this_thread::sleep_for(std::chrono::seconds(1));
    }
  }
}

void AgentFsdbIntegrationBenchmarkHelper::awaitCompletion(
    AgentEnsemble* ensemble) {
  if (writeAllPublishCompleteMarker_) {
    publishCompletionMarker(ensemble);
  }

  if (waitForSubscriptionConnected_) {
    // wait for subscribption to be served
    subscriptionConnected_.wait();
  }

  if (waitForAllPublishConfirmed_) {
    dummyDataPublished_.wait();
  }

  if (FLAGS_json) {
    folly::dynamic fsdbSubscribeLatency = folly::dynamic::object;
    fsdbSubscribeLatency["subscribe_latency_ms"] = subscribe_latency_;

    std::map<std::string, int64_t> fb303Counters;
    if (queryFsdbCounters(fb303Counters)) {
      fsdbSubscribeLatency["publish_time_ms"] =
          fb303Counters["fsdb.publish_time_ms.agent.p99.60"];
      fsdbSubscribeLatency["subscribe_time_ms"] =
          fb303Counters["fsdb.subscribe_time_ms.agent.p99.60"];
    }

    std::cout << toPrettyJson(fsdbSubscribeLatency) << std::endl;
  }

  if (FLAGS_write_agent_config_marker_for_fsdb != "") {
    writeAgentConfigMarkerForFsdb();
  }
}

void AgentFsdbIntegrationBenchmarkHelper::publishCompletionMarker(
    AgentEnsemble* ensemble) {
  ensemble->getSw()->updateStateBlocking(
      "update port desc", [](const auto& state) {
        auto newState = state->clone();
        auto ports = newState->getPorts()->modify(&newState);
        auto port = ports->getNodeIf(kExpectedPortId)->clone();
        port->setDescription(kExpectedDescription);
        HwSwitchMatcher matcher(std::unordered_set<SwitchID>({SwitchID(0)}));
        ports->updateNode(std::move(port), matcher);
        return newState;
      });
  waitForAllPublishConfirmed_ = true;
}

AgentFsdbIntegrationBenchmarkHelper::~AgentFsdbIntegrationBenchmarkHelper() {
  if (pubsubMgr_) {
    pubsubMgr_.reset();
  }
}

} // namespace facebook::fboss
