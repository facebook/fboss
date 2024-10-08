// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/common/PathHelpers.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_common_types.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/Format.h>
#include <folly/String.h>
#include <folly/coro/AsyncGenerator.h>

#include <folly/logging/xlog.h>

#include <functional>

namespace facebook::fboss::fsdb {

enum class SubscriptionState : uint16_t {
  DISCONNECTED,
  DISCONNECTED_GR_HOLD,
  DISCONNECTED_GR_HOLD_EXPIRED,
  CANCELLED,
  CONNECTED,
};

enum class SubscriptionType {
  UNKNOWN = 0,
  PATH = 1,
  DELTA = 2,
  PATCH = 3,
};

static std::unordered_map<SubscriptionType, std::string> subscriptionTypeToStr =
    {
        {SubscriptionType::PATH, "Path"},
        {SubscriptionType::DELTA, "Delta"},
        {SubscriptionType::PATCH, "Patch"},
};

inline bool isConnected(const SubscriptionState& state) {
  return state == SubscriptionState::CONNECTED;
}

inline bool isDisconnected(const SubscriptionState& state) {
  return state == SubscriptionState::DISCONNECTED ||
      state == SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED ||
      state == SubscriptionState::CANCELLED;
}

inline bool isGRHold(const SubscriptionState& state) {
  return state == SubscriptionState::DISCONNECTED_GR_HOLD;
}

inline bool isGRHoldExpired(const SubscriptionState& state) {
  return state == SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED;
}

inline std::string subscriptionStateToString(SubscriptionState state) {
  switch (state) {
    case SubscriptionState::CONNECTED:
      return "CONNECTED";
    case SubscriptionState::DISCONNECTED:
      return "DISCONNECTED";
    case SubscriptionState::DISCONNECTED_GR_HOLD:
      return "DISCONNECTED_GR_HOLD";
    case SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED:
      return "DISCONNECTED_GR_HOLD_EXPIRED";
    case SubscriptionState::CANCELLED:
      return "CANCELLED";
  }
  throw std::runtime_error(
      "Unhandled fsdb::SubscriptionState::" +
      std::to_string(static_cast<int>(state)));
}

using SubscriptionStateChangeCb =
    std::function<void(SubscriptionState, SubscriptionState)>;

struct SubscriptionOptions {
  SubscriptionOptions() = default;
  explicit SubscriptionOptions(
      const std::string& clientId,
      bool subscribeStats = false,
      uint32_t grHoldTimeSec = 0,
      // only mark subscription as CONNECTED on initial sync
      bool requireInitialSyncToMarkConnect = false)
      : clientId_(clientId),
        subscribeStats_(subscribeStats),
        grHoldTimeSec_(grHoldTimeSec),
        requireInitialSyncToMarkConnect_(requireInitialSyncToMarkConnect) {}

  const std::string clientId_;
  bool subscribeStats_{false};
  uint32_t grHoldTimeSec_{0};
  bool requireInitialSyncToMarkConnect_{false};
};

struct SubscriptionInfo {
  std::string server;
  SubscriptionType subscriptionType;
  bool isStats;
  std::vector<std::string> paths;
  FsdbStreamClient::State state;
  FsdbErrorCode disconnectReason;
};

class FsdbSubscriberBase : public FsdbStreamClient {
 public:
  using FsdbStreamClient::FsdbStreamClient;

  virtual SubscriptionInfo getInfo() const = 0;
};

template <typename SubUnit, typename Paths>
class FsdbSubscriber : public FsdbSubscriberBase {
  std::string typeStr() const;
  std::string pathsStr(const Paths& path) const;

 public:
  using FsdbSubUnitUpdateCb = std::function<void(SubUnit&&)>;
  using SubUnitT = SubUnit;

  FsdbSubscriber(
      const std::string& clientId,
      const Paths& subscribePaths,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      FsdbSubUnitUpdateCb operSubUnitUpdate,
      bool subscribeStats,
      std::optional<SubscriptionStateChangeCb> streamStateChangeCb =
          std::nullopt,
      std::optional<FsdbStreamStateChangeCb> connectionStateChangeCb =
          std::nullopt)
      : FsdbSubscriber(
            std::move(SubscriptionOptions(clientId, subscribeStats)),
            subscribePaths,
            streamEvb,
            connRetryEvb,
            operSubUnitUpdate,
            streamStateChangeCb,
            connectionStateChangeCb) {}

  FsdbSubscriber(
      SubscriptionOptions&& options,
      const Paths& subscribePaths,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      FsdbSubUnitUpdateCb operSubUnitUpdate,
      std::optional<SubscriptionStateChangeCb> stateChangeCb = std::nullopt,
      std::optional<FsdbStreamStateChangeCb> connectionStateChangeCb =
          std::nullopt)
      : FsdbSubscriberBase(
            options.clientId_,
            streamEvb,
            connRetryEvb,
            folly::sformat(
                "fsdb{}{}Subscriber_{}",
                typeStr(),
                (options.subscribeStats_ ? "Stat" : "State"),
                pathsStr(subscribePaths)),
            options.subscribeStats_,
            [this](State oldState, State newState) {
              handleConnectionState(oldState, newState);
            }),
        operSubUnitUpdate_(operSubUnitUpdate),
        subscribePaths_(subscribePaths),
        subscriptionOptions_(std::move(options)),
        subscriptionState_(
            subscriptionOptions_.grHoldTimeSec_ > 0
                ? SubscriptionState::DISCONNECTED_GR_HOLD
                : SubscriptionState::DISCONNECTED),
        connectionStateChangeCb_(connectionStateChangeCb),
        subscriptionStateChangeCb_(stateChangeCb),
        staleStateTimer_(folly::AsyncTimeout::make(
            *streamEvb,
            [this]() noexcept { staleStateTimeoutExpired(); })) {
    if (subscriptionOptions_.grHoldTimeSec_ > 0) {
      scheduleStaleStateTimeout();
    }
  }

  virtual ~FsdbSubscriber() override {
    cancelStaleStateTimeout();
  }

  static SubscriptionType subscriptionType();

  SubscriptionInfo getInfo() const override {
    return SubscriptionInfo{
        getServer(),
        subscriptionType(),
        this->isStats(),
        PathHelpers::toStringList(subscribePaths_),
        getState(),
        getDisconnectReason()};
  }

 protected:
  auto createRequest() const {
    if constexpr (std::is_same_v<Paths, std::vector<std::string>>) {
      OperPath operPath;
      operPath.raw() = subscribePaths_;
      OperSubRequest request;
      request.path() = operPath;
      request.subscriberId() = clientId();
      return request;
    } else if constexpr (std::is_same_v<Paths, std::vector<ExtendedOperPath>>) {
      OperSubRequestExtended request;
      request.paths() = subscribePaths_;
      request.subscriberId() = clientId();
      return request;
    }
  }
  SubscriptionState getSubscriptionState() const {
    return *subscriptionState_.rlock();
  }
  void updateSubscriptionState(SubscriptionState newState) {
    auto locked = subscriptionState_.wlock();
    auto oldState = *locked;
    if (oldState == newState) {
      return;
    }
    *locked = newState;
    if ((newState == SubscriptionState::CONNECTED) ||
        (newState == SubscriptionState::CANCELLED)) {
      cancelStaleStateTimeout();
    }
    if (subscriptionStateChangeCb_.has_value()) {
      subscriptionStateChangeCb_.value()(oldState, newState);
    }
  }
  void handleConnectionState(State oldState, State newState) {
    switch (newState) {
      case State::DISCONNECTED:
        if (getSubscriptionState() == SubscriptionState::CONNECTED) {
          if (subscriptionOptions_.grHoldTimeSec_ == 0) {
            // no GR hold, so mark subscription as disconnected immediately
            updateSubscriptionState(SubscriptionState::DISCONNECTED);
            return;
          } else {
            grDisconnectEvents_.add(1);
            scheduleStaleStateTimeout();
            updateSubscriptionState(SubscriptionState::DISCONNECTED_GR_HOLD);
          }
        }
        break;
      case State::CANCELLED:
        updateSubscriptionState(SubscriptionState::CANCELLED);
        break;
      default:
        XLOG(DBG2) << "No-op transition for ConnectionState: "
                   << connectionStateToString(oldState) << " -> "
                   << connectionStateToString(newState);
        break;
    }
    if (connectionStateChangeCb_.has_value()) {
      connectionStateChangeCb_.value()(oldState, newState);
    }
  }

  void scheduleStaleStateTimeout() {
    XLOG(DBG2) << "Scheduling stale state timeout for "
               << subscriptionOptions_.grHoldTimeSec_ << " seconds";
    getStreamEventBase()->runInEventBaseThread([this] {
      if (!staleStateTimer_->isScheduled()) {
        staleStateTimer_->scheduleTimeout(
            std::chrono::seconds(subscriptionOptions_.grHoldTimeSec_));
      }
    });
  }

  void cancelStaleStateTimeout() {
    getStreamEventBase()->runImmediatelyOrRunInEventBaseThreadAndWait([this] {
      if (staleStateTimer_ && staleStateTimer_->isScheduled()) {
        staleStateTimer_->cancelTimeout();
      }
    });
  }

  void staleStateTimeoutExpired() noexcept {
    updateSubscriptionState(SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED);
  }

  const Paths& subscribePaths() const {
    return subscribePaths_;
  }

  const SubscriptionOptions& subscriptionOptions() const {
    return subscriptionOptions_;
  }

  FsdbSubUnitUpdateCb operSubUnitUpdate_;

 private:
  const Paths subscribePaths_;
  SubscriptionOptions subscriptionOptions_;
  folly::Synchronized<SubscriptionState> subscriptionState_;
  std::optional<FsdbStreamStateChangeCb> connectionStateChangeCb_;
  std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb_;
  std::unique_ptr<folly::AsyncTimeout> staleStateTimer_;
  fb303::TimeseriesWrapper grDisconnectEvents_{
      getCounterPrefix() + ".disconnectGRHold",
      fb303::SUM,
      fb303::RATE};
};
} // namespace facebook::fboss::fsdb
