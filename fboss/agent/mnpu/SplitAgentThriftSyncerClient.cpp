/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/SplitAgentThriftSyncerClient.h"

#include <folly/IPAddress.h>
#include <netinet/in.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

namespace {
DEFINE_int32(
    hwagent_reconnect_ms,
    1000,
    "reconnect to swswitch in milliseconds");
} // namespace

namespace facebook::fboss {

SplitAgentThriftClient::SplitAgentThriftClient(
    const std::string& clientId,
    std::shared_ptr<folly::ScopedEventBaseThread> streamEvbThread,
    folly::EventBase* connRetryEvb,
    const std::string& counterPrefix,
    StreamStateChangeCb stateChangeCb,
    uint16_t serverPort,
    SwitchID switchId)
    : ReconnectingThriftClient(
          clientId,
          streamEvbThread->getEventBase(),
          connRetryEvb,
          counterPrefix,
          "multi_switch_streams",
          stateChangeCb,
          FLAGS_hwagent_reconnect_ms),
      streamEvbThread_(streamEvbThread),
      serverPort_(serverPort),
      switchId_(switchId) {
  setServerOptions(ServerOptions("::1", serverPort_));
  scheduleTimeout();
}

SplitAgentThriftClient::~SplitAgentThriftClient() {}

void SplitAgentThriftClient::connectClient(const ServerOptions& options) {
  auto channel = apache::thrift::PooledRequestChannel::newChannel(
      streamEvbThread_->getEventBase(),
      streamEvbThread_,
      [options = options](folly::EventBase& evb) mutable {
        return apache::thrift::RocketClientChannel::newChannel(
            folly::AsyncSocket::UniquePtr(
                new folly::AsyncSocket(&evb, options.dstAddr)));
      });

  multiSwitchClient_.reset(
      new apache::thrift::Client<multiswitch::MultiSwitchCtrl>(
          std::move(channel)));
}

void SplitAgentThriftClient::connectToServer(const ServerOptions& options) {
  try {
    connectClient(options);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Connect to server failed with ex:" << ex.what();
    setState(State::DISCONNECTED);
    return;
  }
  try {
    startClientService();
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error while starting stream or sink from " << clientId()
              << " : " << ex.what();
    setState(State::DISCONNECTED);
    return;
  }
  // set state to connecting. The event loop will start the sink generator
  // and move state to connected
  setState(State::CONNECTING);
}

#if FOLLY_HAS_COROUTINES
folly::coro::Task<void> SplitAgentThriftClient::serviceLoopWrapper() {
  setState(State::CONNECTED);
  try {
    co_await serveStream();
  } catch (const folly::OperationCancelled&) {
    setState(State::CANCELLED);
  } catch (const std::exception& ex) {
    XLOG(ERR) << "Error while notifying events from " << clientId() << " : "
              << ex.what();
    setState(State::DISCONNECTED);
  }
  co_return;
}
#endif

apache::thrift::Client<multiswitch::MultiSwitchCtrl>*
SplitAgentThriftClient::getThriftClient() {
  return multiSwitchClient_.get();
}

void SplitAgentThriftClient::resetClient() {
  multiSwitchClient_.reset();
}

template <typename CallbackObjectT>
ThriftSinkClient<CallbackObjectT>::ThriftSinkClient(
    folly::StringPiece name,
    uint16_t serverPort,
    SwitchID switchId,
    ThriftSinkConnectFn connectFn,
    std::shared_ptr<folly::ScopedEventBaseThread> eventThread,
    folly::EventBase* retryEvb)
    : SplitAgentThriftClient(
          std::string(name),
          eventThread,
          retryEvb,
          std::string(name),
          [](State, State) {},
          serverPort,
          switchId),
      connectFn_(std::move(connectFn)) {}

template <typename CallbackObjectT>
void ThriftSinkClient<CallbackObjectT>::startClientService() {
  sinkClient_.reset(new EventNotifierSinkClient(
      connectFn_(getSwitchId(), getThriftClient())));
}

template <typename CallbackObjectT>
void ThriftSinkClient<CallbackObjectT>::resetClient() {
  sinkClient_.reset();
  SplitAgentThriftClient::resetClient();
}

#if FOLLY_HAS_COROUTINES
template <typename CallbackObjectT>
folly::coro::Task<void> ThriftSinkClient<CallbackObjectT>::serveStream() {
  co_await sinkClient_->sink(
      [&]() -> folly::coro::AsyncGenerator<CallbackObjectT&&> {
        while (true) {
          auto event = co_await eventsQueue_.dequeue();
          if (isCancelled()) {
            co_return;
          }
          co_yield std::move(event);
        }
      }());
  co_return;
}
#endif

template <typename CallbackObjectT>
ThriftSinkClient<CallbackObjectT>::~ThriftSinkClient() {
  CHECK(isCancelled());
}

template <typename StreamObjectT>
ThriftStreamClient<StreamObjectT>::ThriftStreamClient(
    folly::StringPiece name,
    uint16_t serverPort,
    SwitchID switchId,
#if FOLLY_HAS_COROUTINES
    ThriftStreamConnectFn connectFn,
#endif
    EventHandlerFn eventHandlerFn,
    HwSwitch* hw,
    std::shared_ptr<folly::ScopedEventBaseThread> eventThread,
    folly::EventBase* retryEvb)
    : SplitAgentThriftClient(
          std::string(name),
          eventThread,
          retryEvb,
          std::string(name),
          [](State, State) {},
          serverPort,
          switchId),
#if FOLLY_HAS_COROUTINES
      connectFn_(std::move(connectFn)),
#endif
      eventHandlerFn_(std::move(eventHandlerFn)),
      hw_(hw) {
}

template <typename StreamObjectT>
void ThriftStreamClient<StreamObjectT>::startClientService() {
#if FOLLY_HAS_COROUTINES
  streamClient_.reset(new EventNotifierStreamClient(
      connectFn_(getSwitchId(), getThriftClient())));
#endif
}

#if FOLLY_HAS_COROUTINES
template <typename StreamObjectT>
folly::coro::Task<void> ThriftStreamClient<StreamObjectT>::serveStream() {
  while (const auto& event = co_await streamClient_->next()) {
    if (isCancelled()) {
      co_return;
    }
    auto eventObj = *event;
    eventHandlerFn_(eventObj, hw_);
  }
  co_return;
}
#endif

template <typename StreamObjectT>
void ThriftStreamClient<StreamObjectT>::resetClient() {
#if FOLLY_HAS_COROUTINES
  streamClient_.reset();
#endif
  SplitAgentThriftClient::resetClient();
}

template <typename StreamObjectT>
ThriftStreamClient<StreamObjectT>::~ThriftStreamClient() {
  CHECK(isCancelled());
}

template class ThriftSinkClient<multiswitch::LinkEvent>;
template class ThriftSinkClient<multiswitch::FdbEvent>;
template class ThriftSinkClient<multiswitch::RxPacket>;
template class ThriftStreamClient<multiswitch::TxPacket>;
template class ThriftSinkClient<multiswitch::HwSwitchStats>;

} // namespace facebook::fboss
