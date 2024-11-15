// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/synchronization/Baton.h>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"

namespace facebook::fboss::fsdb::test {

class FsdbBenchmarkTestHelper {
 public:
  void setup();
  void publishPath(const AgentStats& stats, uint64_t stamp);
  void startPublisher();
  void stopPublisher(bool gr = false);
  void TearDown();
  void waitForPublisherConnected();
  bool isPublisherConnected() {
    return readyForPublishing_.load();
  }
  std::optional<fboss::fsdb::FsdbOperTreeMetadata> getPublisherRootMetadata(
      bool isStats);
  fsdb::test::FsdbTestServer& testServer() {
    return *fsdbTestServer_;
  }

 private:
  std::vector<std::string> getAgentStatsPath();

  std::unique_ptr<fsdb::test::FsdbTestServer> fsdbTestServer_;
  std::unique_ptr<fsdb::FsdbPubSubManager> pubsubMgr_;
  folly::Baton<> publisherConnected_;
  std::atomic_bool readyForPublishing_ = false;
};

} // namespace facebook::fboss::fsdb::test
