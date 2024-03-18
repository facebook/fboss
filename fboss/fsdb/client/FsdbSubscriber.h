// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include "fboss/fsdb/client/FsdbStreamClient.h"
#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

#include <folly/Format.h>
#include <folly/String.h>
#include <folly/experimental/coro/AsyncGenerator.h>

#include <folly/logging/xlog.h>

#include <functional>

namespace facebook::fboss::fsdb {

std::string extendedPathsStr(const std::vector<ExtendedOperPath>& path);

enum class SubscriptionState : uint16_t {
  DISCONNECTED,
  DISCONNECTED_GR_HOLD_EXPIRED,
  CANCELLED,
  CONNECTED,
  CONNECTED_GR_HOLD,
};

inline bool isConnected(SubscriptionState state) {
  return (state == SubscriptionState::CONNECTED) ||
      (state == SubscriptionState::CONNECTED_GR_HOLD);
}

inline bool isDisconnected(SubscriptionState state) {
  return (state == SubscriptionState::DISCONNECTED) ||
      (state == SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED) ||
      (state == SubscriptionState::CANCELLED);
}

inline std::string subscriptionStateToString(SubscriptionState state) {
  switch (state) {
    case SubscriptionState::CONNECTED:
      return "CONNECTED";
    case SubscriptionState::DISCONNECTED:
      return "DISCONNECTED";
    case SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED:
      return "DISCONNECTED_GR_HOLD_EXPIRED";
    case SubscriptionState::CONNECTED_GR_HOLD:
      return "CONNECTED_GR_HOLD";
    case SubscriptionState::CANCELLED:
      return "CANCELLED";
  }
  throw std::runtime_error(
      "Unhandled fsdb::SubscriptionState::" +
      std::to_string(static_cast<int>(state)));
}

using SubscriptionStateChangeCb =
    std::function<void(SubscriptionState, SubscriptionState)>;

template <typename SubUnit, typename PathElement>
class FsdbSubscriber : public FsdbStreamClient {
  using Paths = std::vector<PathElement>;
  std::string typeStr() const;
  std::string pathsStr(const Paths& path) const;

 public:
  struct SubscriptionOptions {
    SubscriptionOptions() = default;
    explicit SubscriptionOptions(
        const std::string& clientId,
        bool subscribeStats = false,
        uint32_t grHoldTimeSec = 0)
        : clientId_(clientId),
          subscribeStats_(subscribeStats),
          grHoldTimeSec_(grHoldTimeSec) {}

    const std::string clientId_;
    bool subscribeStats_{false};
    uint32_t grHoldTimeSec_{0};
  };

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
      : FsdbStreamClient(
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
        connectionStateChangeCb_(connectionStateChangeCb),
        subscriptionStateChangeCb_(stateChangeCb),
        staleStateTimer_(folly::AsyncTimeout::make(
            *connRetryEvb,
            [this]() noexcept { staleStateTimeoutExpired(); })) {}

 protected:
  auto createRequest() const {
    if constexpr (std::is_same_v<PathElement, std::string>) {
      OperPath operPath;
      operPath.raw() = subscribePaths_;
      OperSubRequest request;
      request.path() = operPath;
      request.subscriberId() = clientId();
      return request;
    } else {
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
    if (staleStateTimer_ && staleStateTimer_->isScheduled()) {
      if ((newState == SubscriptionState::CONNECTED) ||
          (newState == SubscriptionState::CANCELLED)) {
        connRetryEvb_->runImmediatelyOrRunInEventBaseThreadAndWait(
            [this] { staleStateTimer_->cancelTimeout(); });
      }
    }
    if (subscriptionStateChangeCb_.has_value()) {
      subscriptionStateChangeCb_.value()(oldState, newState);
    }
  }
  void handleConnectionState(State oldState, State newState) {
    if (newState == State::DISCONNECTED) {
      if (isDisconnected(getSubscriptionState())) {
        return;
      } else if (subscriptionOptions_.grHoldTimeSec_ == 0) {
        // no GR hold, so mark subscription as disconnected immediately
        updateSubscriptionState(SubscriptionState::DISCONNECTED);
        return;
      } else if (getSubscriptionState() == SubscriptionState::CONNECTED) {
        grDisconnectEvents_.add(1);
        connRetryEvb_->runInEventBaseThread([this] {
          staleStateTimer_->scheduleTimeout(
              std::chrono::seconds(subscriptionOptions_.grHoldTimeSec_));
        });
        updateSubscriptionState(SubscriptionState::CONNECTED_GR_HOLD);
      }
    } else if (newState == State::CANCELLED) {
      updateSubscriptionState(SubscriptionState::CANCELLED);
    } else {
      XLOG(DBG2) << "FsdbScubscriber: no-op for ConnectionState: "
                 << connectionStateToString(newState);
    }
    if (connectionStateChangeCb_.has_value()) {
      connectionStateChangeCb_.value()(oldState, newState);
    }
  }

  void staleStateTimeoutExpired() noexcept {
    updateSubscriptionState(SubscriptionState::DISCONNECTED_GR_HOLD_EXPIRED);
  }

  FsdbSubUnitUpdateCb operSubUnitUpdate_;

 private:
  const Paths subscribePaths_;
  SubscriptionOptions subscriptionOptions_;
  folly::Synchronized<SubscriptionState> subscriptionState_{
      SubscriptionState::DISCONNECTED};
  std::optional<FsdbStreamStateChangeCb> connectionStateChangeCb_;
  std::optional<SubscriptionStateChangeCb> subscriptionStateChangeCb_;
  std::unique_ptr<folly::AsyncTimeout> staleStateTimer_;
  fb303::TimeseriesWrapper grDisconnectEvents_{
      getCounterPrefix() + ".disconnectGRHold",
      fb303::SUM,
      fb303::RATE};
};
} // namespace facebook::fboss::fsdb
