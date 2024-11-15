// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/benchmarks/FsdbBenchmarkTestHelper.h"

namespace {

const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStateRoot>
    stateRoot;
const thriftpath::RootThriftPath<facebook::fboss::fsdb::FsdbOperStatsRoot>
    statsRoot;
const std::vector<std::string> kPublishRoot{"agent"};

} // anonymous namespace

namespace facebook::fboss::fsdb::test {

void FsdbBenchmarkTestHelper::setup() {
  fsdbTestServer_ = std::make_unique<fsdb::test::FsdbTestServer>();
  FLAGS_fsdbPort = fsdbTestServer_->getFsdbPort();
  const std::string clientId = "agent";
  pubsubMgr_ = std::make_unique<fsdb::FsdbPubSubManager>(clientId);
}

void FsdbBenchmarkTestHelper::publishPath(
    const AgentStats& stats,
    uint64_t stamp) {
  fsdb::OperState state;
  state.set_contents(
      apache::thrift::BinarySerializer::serialize<std::string>(stats));
  state.protocol() = fsdb::OperProtocol::BINARY;
  state.metadata() = fsdb::OperMetadata();
  state.metadata()->lastConfirmedAt() = stamp;
  pubsubMgr_->publishStat(std::move(state));
}

void FsdbBenchmarkTestHelper::startPublisher() {
  auto stateChangeCb = [this](
                           fsdb::FsdbStreamClient::State /* oldState */,
                           fsdb::FsdbStreamClient::State newState) {
    if (newState == fsdb::FsdbStreamClient::State::CONNECTED) {
      publisherConnected_.post();
      readyForPublishing_.store(true);
    } else {
      readyForPublishing_.store(false);
    }
  };
  // agent is Path publisher for stats
  pubsubMgr_->createStatPathPublisher(
      getAgentStatsPath(), std::move(stateChangeCb));
}

void FsdbBenchmarkTestHelper::waitForPublisherConnected() {
  publisherConnected_.wait();
}

void FsdbBenchmarkTestHelper::stopPublisher(bool gr) {
  readyForPublishing_.store(false);
  pubsubMgr_->removeStatPathPublisher(gr);
}

void FsdbBenchmarkTestHelper::TearDown() {
  fsdbTestServer_.reset();
  if (pubsubMgr_) {
    stopPublisher();
    CHECK(!readyForPublishing_.load());
    pubsubMgr_.reset();
  }
}

std::optional<fboss::fsdb::FsdbOperTreeMetadata>
FsdbBenchmarkTestHelper::getPublisherRootMetadata(bool isStats) {
  auto metadata =
      fsdbTestServer_->getPublisherRootMetadata(*kPublishRoot.begin(), isStats);
  return metadata;
}

std::vector<std::string> FsdbBenchmarkTestHelper::getAgentStatsPath() {
  return statsRoot.agent().tokens();
}

} // namespace facebook::fboss::fsdb::test
