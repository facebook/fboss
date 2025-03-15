// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <folly/synchronization/Baton.h>

#include "fboss/fsdb/client/FsdbPubSubManager.h"
#include "fboss/fsdb/tests/utils/FsdbTestServer.h"

namespace facebook::fboss::fsdb::test {

class FsdbBenchmarkTestHelper {
 public:
  void setup(int32_t numSubscriptions = 1);
  void publishPath(const AgentStats& stats, uint64_t stamp);
  void publishStatePatch(const state::SwitchState& state, uint64_t stamp);
  void startPublisher(bool stats = true);
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
  void addSubscription(
      const fsdb::FsdbStateSubscriber::FsdbOperStateUpdateCb& stateUpdateCb);
  void addStatePatchSubscription(
      const fsdb::FsdbPatchSubscriber::FsdbOperPatchUpdateCb& stateUpdateCb,
      int32_t subscriberID);
  void removeSubscription(bool stats = true, int32_t subscriberID = 0);
  bool isSubscriptionConnected(int32_t subscriberID = 0);
  void waitForAllSubscribersConnected(int32_t numSubscribers = 1);

 private:
  std::vector<std::string> getAgentStatsPath();

  std::unique_ptr<fsdb::test::FsdbTestServer> fsdbTestServer_;
  std::vector<std::unique_ptr<fsdb::FsdbPubSubManager>> pubsubMgrs_;
  folly::Baton<> publisherConnected_;
  std::atomic_bool readyForPublishing_ = false;
  std::vector<std::atomic_bool> subscriptionConnected_;
};

} // namespace facebook::fboss::fsdb::test
