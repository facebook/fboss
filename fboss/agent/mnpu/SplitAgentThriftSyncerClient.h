/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once
#include <memory>

#include <folly/CancellationToken.h>
#include <folly/io/async/EventBase.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include <gflags/gflags.h>
#include <string>

#if FOLLY_HAS_COROUTINES
#include <folly/experimental/coro/AsyncScope.h>
#include <folly/experimental/coro/UnboundedQueue.h>
#endif

#include "fboss/agent/MultiSwitchThriftHandler.h"
#include "fboss/lib/CommonThriftUtils.h"

namespace facebook::fboss {

class HwSwitch;

class SplitAgentThriftClient : public ReconnectingThriftClient {
 public:
  SplitAgentThriftClient(
      const std::string& clientId,
      std::shared_ptr<folly::ScopedEventBaseThread> streamEvbThread,
      folly::EventBase* connRetryEvb,
      const std::string& counterPrefix,
      StreamStateChangeCb stateChangeCb,
      uint16_t serverPort,
      SwitchID switchId);
  ~SplitAgentThriftClient() override;

 protected:
  void connectClient(const ServerOptions& options);
  void resetClient() override;
  apache::thrift::Client<multiswitch::MultiSwitchCtrl>* getThriftClient();
  virtual void startClientService() = 0;
  void connectToServer(const ServerOptions& options) override;
#if FOLLY_HAS_COROUTINES
  virtual folly::coro::Task<void> serveStream() = 0;
#endif
  SwitchID getSwitchId() {
    return switchId_;
  }

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> serviceLoopWrapper() override;
#endif

  std::shared_ptr<folly::ScopedEventBaseThread> streamEvbThread_;
  uint32_t serverPort_;
  SwitchID switchId_;
  std::unique_ptr<apache::thrift::Client<multiswitch::MultiSwitchCtrl>>
      multiSwitchClient_;
};

template <typename CallbackObjectT>
class ThriftSinkClient : public SplitAgentThriftClient {
 public:
  using EventNotifierSinkClient =
      apache::thrift::ClientSink<CallbackObjectT, bool>;
  using ThriftSinkConnectFn = std::function<EventNotifierSinkClient(
      SwitchID,
      apache::thrift::Client<multiswitch::MultiSwitchCtrl>*)>;

  ThriftSinkClient(
      folly::StringPiece name,
      uint16_t serverPort,
      SwitchID switchId,
      ThriftSinkConnectFn connectFn,
      std::shared_ptr<folly::ScopedEventBaseThread> eventThread,
      folly::EventBase* connRetryEvb);
  ~ThriftSinkClient() override;
  void resetClient() override;
  void enqueue(CallbackObjectT callbackObject) {
#if FOLLY_HAS_COROUTINES
    eventsQueue_.enqueue(std::move(callbackObject));
#endif
  }
  void startClientService() override;

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> serveStream() override;
  folly::coro::UnboundedQueue<
      CallbackObjectT,
      true /*SingleProducer*/,
      true /* SingleConsumer*/>
      eventsQueue_;
#endif
  std::unique_ptr<EventNotifierSinkClient> sinkClient_;
  ThriftSinkConnectFn connectFn_;
  int32_t serverPort_;
};

template <typename StreamObjectT>
class ThriftStreamClient : public SplitAgentThriftClient {
 public:
#if FOLLY_HAS_COROUTINES
  using EventNotifierStreamClient =
      folly::coro::AsyncGenerator<StreamObjectT&&, StreamObjectT>;
  using ThriftStreamConnectFn = std::function<EventNotifierStreamClient(
      SwitchID,
      apache::thrift::Client<multiswitch::MultiSwitchCtrl>*)>;
#endif
  using EventHandlerFn = std::function<void(StreamObjectT&, HwSwitch*)>;

  ThriftStreamClient(
      folly::StringPiece name,
      uint16_t serverPort,
      SwitchID switchId,
#if FOLLY_HAS_COROUTINES
      ThriftStreamConnectFn connectFn,
#endif
      EventHandlerFn eventHandlerFn,
      HwSwitch* hw,
      std::shared_ptr<folly::ScopedEventBaseThread> eventThread,
      folly::EventBase* retryEvb);
  ~ThriftStreamClient() override;
  void resetClient() override;
  void startClientService() override;

 private:
#if FOLLY_HAS_COROUTINES
  folly::coro::Task<void> serveStream() override;

  std::unique_ptr<EventNotifierStreamClient> streamClient_;
  ThriftStreamConnectFn connectFn_;
#endif

  EventHandlerFn eventHandlerFn_;
  HwSwitch* hw_;
};
} // namespace facebook::fboss
