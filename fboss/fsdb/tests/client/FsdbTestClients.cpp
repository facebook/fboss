// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/fsdb/tests/client/FsdbTestClients.h"
#include <gtest/gtest.h>

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

namespace facebook::fboss::fsdb::test {

TestFsdbStreamClient::TestFsdbStreamClient(
    folly::EventBase* streamEvb,
    folly::EventBase* timerEvb)
    : FsdbStreamClient(
          "test_fsdb_client",
          streamEvb,
          timerEvb,
          "test_fsdb_client",
          false,
          [this](auto oldState, auto newState) {
            EXPECT_NE(oldState, newState);
            lastStateUpdateSeen_ = newState;
          }) {}

TestFsdbStreamClient::~TestFsdbStreamClient() {
  cancel();
}

folly::coro::Task<TestFsdbStreamClient::StreamT>
TestFsdbStreamClient::setupStream() {
  co_return StreamT();
}

folly::coro::Task<void> TestFsdbStreamClient::serveStream(
    StreamT&& /* stream */) {
  /*
   * Dummy generator for testing. Derived classes will
   * provide this as we flesh out publisher, subscriber
   * classes
   */
  auto gen = []() -> folly::coro::AsyncGenerator<int> {
    auto i = 0;
    while (true) {
      co_yield ++i;
    }
  }();
  while (auto intgen = co_await gen.next()) {
    if (isCancelled()) {
      XLOG(DBG2) << " Detected cancellation";
      break;
    }
    // Make a thrift call to detect connections getting
    // dropped.
    co_await client_->co_getOperSubscriberInfos({});
  }
  XLOG(DBG2) << "Client Cancellation Completed";
  co_return;
}

OperDelta makeDelta(const cfg::AgentConfig& config) {
  OperPath deltaPath;
  thriftpath::RootThriftPath<FsdbOperStateRoot> root;
  deltaPath.raw() = root.agent().config().tokens();
  OperDeltaUnit deltaUnit;
  deltaUnit.path() = deltaPath;
  deltaUnit.newState() =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(config);
  OperDelta delta;
  delta.changes()->push_back(deltaUnit);
  delta.protocol() = OperProtocol::SIMPLE_JSON;
  return delta;
}

OperDelta makeDelta(
    const folly::F14FastMap<std::string, HwPortStats>& portStats) {
  OperPath deltaPath;
  thriftpath::RootThriftPath<FsdbOperStatsRoot> root;
  deltaPath.raw() = root.agent().hwPortStats().tokens();
  OperDeltaUnit deltaUnit;
  deltaUnit.path() = deltaPath;
  deltaUnit.newState() =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(portStats);
  OperDelta delta;
  delta.changes()->push_back(deltaUnit);
  delta.protocol() = OperProtocol::SIMPLE_JSON;
  return delta;
}

OperState makeState(const cfg::AgentConfig& config) {
  AgentData root;
  root.config() = config;
  OperState stateUnit;
  stateUnit.contents() =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(root);
  stateUnit.protocol() = OperProtocol::SIMPLE_JSON;
  return stateUnit;
}

OperState makeState(
    const folly::F14FastMap<std::string, HwPortStats>& portStats) {
  AgentStats root;
  root.hwPortStats() = portStats;
  OperState stateUnit;
  stateUnit.contents() =
      apache::thrift::SimpleJSONSerializer::serialize<std::string>(root);
  stateUnit.protocol() = OperProtocol::SIMPLE_JSON;
  return stateUnit;
}
cfg::AgentConfig makeAgentConfig(
    const std::map<std::string, std::string>& cmdLinArgs) {
  cfg::AgentConfig cfg;
  cfg.defaultCommandLineArgs() = cmdLinArgs;
  return cfg;
}

folly::F14FastMap<std::string, HwPortStats> makePortStats(
    int64_t inBytes,
    const std::string& portName) {
  folly::F14FastMap<std::string, HwPortStats> portStats;
  HwPortStats stats;
  stats.inBytes_() = inBytes;
  portStats[portName] = stats;
  return portStats;
}

Patch makePatch(const cfg::AgentConfig& config) {
  using AgentDataMembers = apache::thrift::reflect_struct<AgentData>::member;
  using FsdbOperStateRootMembers =
      apache::thrift::reflect_struct<FsdbOperStateRoot>::member;
  folly::IOBufQueue queue;
  apache::thrift::CompactSerializer::serialize(config, &queue);
  thrift_cow::PatchNode val;
  val.set_val(queue.moveAsValue());

  thrift_cow::StructPatch s;
  s.children() = {{AgentDataMembers::config::id::value, std::move(val)}};

  thrift_cow::PatchNode root;
  root.set_struct_node(std::move(s));
  Patch p;
  p.patch() = std::move(root);
  p.basePath() = {
      folly::to<std::string>(FsdbOperStateRootMembers::agent::id::value)};
  return p;
}

Patch makePatch(const folly::F14FastMap<std::string, HwPortStats>& portStats) {
  using AgentStatsMembers = apache::thrift::reflect_struct<AgentStats>::member;
  using FsdbOperStatsRootMembers =
      apache::thrift::reflect_struct<FsdbOperStatsRoot>::member;
  folly::IOBufQueue queue;
  apache::thrift::CompactSerializer::serialize(portStats, &queue);
  thrift_cow::PatchNode val;
  val.set_val(queue.moveAsValue());

  thrift_cow::StructPatch s;
  s.children() = {{AgentStatsMembers::hwPortStats::id::value, std::move(val)}};

  thrift_cow::PatchNode root;
  root.set_struct_node(std::move(s));
  Patch p;
  p.patch() = std::move(root);
  p.basePath() = {"agent"};
  return p;
}

} // namespace facebook::fboss::fsdb::test
