// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/Synchronized.h>
#include <folly/container/F14Map.h>
#include <folly/logging/xlog.h>

#include "fboss/fsdb/client/FsdbDeltaPublisher.h"
#include "fboss/fsdb/client/FsdbDeltaSubscriber.h"
#include "fboss/fsdb/client/FsdbStatePublisher.h"
#include "fboss/fsdb/client/FsdbStateSubscriber.h"
#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/FsdbModel.h"

#include "fboss/lib/CommonUtils.h"

namespace facebook::fboss::fsdb::test {

class TestFsdbStreamClient : public FsdbStreamClient {
 public:
  TestFsdbStreamClient(folly::EventBase* streamEvb, folly::EventBase* timerEvb);
  ~TestFsdbStreamClient() override;

  std::optional<FsdbStreamClient::State> lastStateUpdateSeen() const {
    return lastStateUpdateSeen_;
  }

 private:
  folly::coro::Task<StreamT> setupStream() override;
  folly::coro::Task<void> serveStream(StreamT&& /* stream */) override;

  std::optional<FsdbStreamClient::State> lastStateUpdateSeen_;
};

OperDelta makeDelta(const cfg::AgentConfig& cfg);
/*
 * makeState serializes FsdbOperStateRoot, after updating
 * agent config inside the root object
 */
OperState makeState(const cfg::AgentConfig& cfg);

OperDelta makeDelta(
    const folly::F14FastMap<std::string, HwPortStats>& portStats);
/*
 * makeState serializes FsdbOperStatsRoot, after updating
 * agent config inside the root object
 */
OperState makeState(
    const folly::F14FastMap<std::string, HwPortStats>& portStats);

cfg::AgentConfig makeAgentConfig(
    const std::map<std::string, std::string>& cmdLinArgs);

folly::F14FastMap<std::string, HwPortStats> makePortStats(
    int64_t inBytes,
    const std::string& portName = "eth1/1/1");

template <typename PubSubT>
class TestFsdbSubscriber : public PubSubT::SubscriberT {
  using SubUnitT = typename PubSubT::SubUnitT;
  using BaseT = typename PubSubT::SubscriberT;

 public:
  TestFsdbSubscriber(
      const std::string& clientId,
      const std::vector<std::string>& subscribePath,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      bool subscribeStats = false)
      : BaseT(
            clientId,
            subscribePath,
            streamEvb,
            connRetryEvb,
            [this](SubUnitT&& unit) {
              initialSyncDone_ = true;
              publishedQueue_.wlock()->emplace_back(std::move(unit));
            },
            subscribeStats,
            [this](auto /*oldState*/, auto newState) {
              if (newState != FsdbStreamClient::State::CONNECTED) {
                XLOG(DBG2) << " Subscriber: " << this->clientId()
                           << " not connected";
                initialSyncDone_ = false;
              }
            }) {}
  ~TestFsdbSubscriber() override {
    this->cancel();
  }
  size_t queueSize() const {
    return publishedQueue_.rlock()->size();
  }
  std::vector<SubUnitT> queuedUnits() const {
    return *publishedQueue_.rlock();
  }
  bool initialSyncDone() const {
    return initialSyncDone_;
  }
  void assertQueue(int expectedSize, int retries = 60) const {
    WITH_RETRIES_N(retries, {
      ASSERT_EVENTUALLY_EQ(queueSize(), expectedSize);
      for (const auto& unit : queuedUnits()) {
        ASSERT_GT(*unit.metadata()->lastConfirmedAt(), 0);
      }
    });
  }

 private:
  folly::Synchronized<std::vector<SubUnitT>> publishedQueue_;
  bool initialSyncDone_{false};
};

struct DeltaSubT {
  using SubUnitT = OperDelta;
  using SubscriberT = FsdbDeltaSubscriber;
};
struct StateSubT {
  using SubUnitT = OperState;
  using SubscriberT = FsdbStateSubscriber;
};
struct StateExtSubT {
  using SubUnitT = OperState;
  using SubscriberT = FsdbExtStateSubscriber;
};
using TestFsdbDeltaSubscriber = TestFsdbSubscriber<DeltaSubT>;
using TestFsdbStateSubscriber = TestFsdbSubscriber<StateSubT>;
using TestFsdbExtStateSubscriber = TestFsdbSubscriber<StateExtSubT>;

template <bool pubSubStats>
struct DeltaPubSubT {
  using PubUnitT = OperDelta;
  using PublisherT = FsdbDeltaPublisher;
  using SubUnitT = OperDelta;
  using SubscriberT = TestFsdbDeltaSubscriber;
  static bool constexpr PubSubStats = pubSubStats;
};
using DeltaPubSubForStats = DeltaPubSubT<true>;
using DeltaPubSubForState = DeltaPubSubT<false>;
template <bool pubSubStats>
struct StatePubSubT {
  using PubUnitT = OperState;
  using PublisherT = FsdbStatePublisher;
  using SubUnitT = PubUnitT;
  using SubscriberT = TestFsdbStateSubscriber;
  static bool constexpr PubSubStats = pubSubStats;
};

using StatePubSubForStats = StatePubSubT<true>;
using StatePubSubForState = StatePubSubT<false>;
} // namespace facebook::fboss::fsdb::test
