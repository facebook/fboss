/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/mnpu/OperDeltaSyncer.h"
#include "fboss/agent/HwSwitch.h"

#include <folly/IPAddress.h>
#include <netinet/in.h>
#include <thrift/lib/cpp2/async/PooledRequestChannel.h>
#include <thrift/lib/cpp2/async/ReconnectingRequestChannel.h>
#include <thrift/lib/cpp2/async/RetryingRequestChannel.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>
#include <memory>

DEFINE_int32(
    oper_sync_timeout,
    0x7fffffff, /* 24.86 days - max supported by setTimeout api */
    "idle timeout for oper sync client in milliseconds");

namespace facebook::fboss {

OperDeltaSyncer::OperDeltaSyncer(
    uint16_t serverPort,
    SwitchID switchId,
    HwSwitch* hw)
    : serverPort_(serverPort),
      switchId_(switchId),
      hw_(hw),
      operSyncEvbThread_(new folly::ScopedEventBaseThread("OperSyncThread")) {
  initOperDeltaSync();
}

void OperDeltaSyncer::initOperDeltaSync() {
  auto reconnectingChannel =
      apache::thrift::PooledRequestChannel::newSyncChannel(
          operSyncEvbThread_, [this](folly::EventBase& evb) {
            return apache::thrift::RetryingRequestChannel::newChannel(
                evb,
                2, /*retries before error*/
                apache::thrift::ReconnectingRequestChannel::newChannel(
                    *operSyncEvbThread_->getEventBase(),
                    [this](folly::EventBase& evb) {
                      auto socket = folly::AsyncSocket::UniquePtr(
                          new folly::AsyncSocket(&evb));
                      socket->connect(
                          nullptr, folly::SocketAddress("::1", serverPort_));
                      auto channel =
                          apache::thrift::RocketClientChannel::newChannel(
                              std::move(socket));
                      channel->setTimeout(FLAGS_oper_sync_timeout);
                      return channel;
                    }));
          });
  operSyncClient_.reset(
      new apache::thrift::Client<multiswitch::MultiSwitchCtrl>(
          std::move(reconnectingChannel)));
}

void OperDeltaSyncer::startOperSync() {
  operSyncRunning_.store(true);
  operSyncThread_ = std::make_unique<std::thread>([this]() { operSyncLoop(); });
}

void OperDeltaSyncer::operSyncLoop() {
  auto lastUpdateResult = fsdb::OperDelta();
  bool initialSync{true};
  while (operSyncRunning_.load()) {
    try {
      multiswitch::StateOperDelta lastOperDeltaResult;
      lastOperDeltaResult.operDelta() = lastUpdateResult;
      multiswitch::StateOperDelta stateOperDelta;
      operSyncClient_->sync_getNextStateOperDelta(
          stateOperDelta, switchId_, lastOperDeltaResult, initialSync);
      // SwSwitch can send empty operdelta when cancelling the service on
      // shutdown
      if (operSyncRunning_.load() &&
          stateOperDelta.operDelta()->changes()->size()) {
        lastUpdateResult = hw_->stateChanged(*stateOperDelta.operDelta());

        // TODO - transition HwSwitch state based on notification from SwSwitch
        if (hw_->getRunState() != SwitchRunState::CONFIGURED) {
          hw_->switchRunStateChanged(SwitchRunState::CONFIGURED);
        }
      }
      initialSync = false;
    } catch (const std::exception& ex) {
      XLOG_EVERY_MS(ERR, 5000)
          << fmt::format("Failed to get next oper delta: {}", ex.what());
    }
  }
}

void OperDeltaSyncer::stopOperSync() {
  // stop any new requests
  operSyncRunning_.store(false);
  // send exit notification to server with short timeout
  try {
    apache::thrift::RpcOptions options;
    options.setTimeout(std::chrono::milliseconds(1000));
    operSyncClient_->sync_gracefulExit(options, switchId_);
  } catch (const std::exception& ex) {
    XLOG(ERR) << fmt::format(
        "Failed to send graceful exit notification to swswitch: {}", ex.what());
  }
  if (operSyncThread_) {
    operSyncThread_->join();
  }
}
} // namespace facebook::fboss
