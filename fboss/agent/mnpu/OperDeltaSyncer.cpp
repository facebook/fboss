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
#include "fboss/agent/AgentFeatures.h"
#include "fboss/agent/HwSwitch.h"
#include "fboss/agent/state/StateDelta.h"

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
  int64_t lastUpdateSeqNum{0};
  while (operSyncRunning_.load()) {
    multiswitch::StateOperDelta lastOperDeltaResult;
    lastOperDeltaResult.operDelta() = lastUpdateResult;
    multiswitch::StateOperDelta stateOperDelta;
    apache::thrift::RpcOptions options;
    /*
     * Set timeout in RPC call to avoid forever blocking wait when server
     * crashes. The timeout on client is set to be more than the timeout on
     * server side wait for next oper delta to be available so that in a
     * normal sequence of operation, client gets an empty delta periodically
     * and reconnects when new state update is not available. The client side
     * timeout should happen only when the server crashes.
     */
    options.setTimeout(std::chrono::seconds(2 * FLAGS_oper_sync_req_timeout));
    try {
      operSyncClient_->sync_getNextStateOperDelta(
          options,
          stateOperDelta,
          switchId_,
          lastOperDeltaResult,
          lastUpdateSeqNum);
    } catch (const apache::thrift::transport::TTransportException& ex) {
      if (ex.getType() ==
          apache::thrift::transport::TTransportException::TIMED_OUT) {
        XLOG(DBG2) << "Timed out waiting for next oper delta from swswitch";
      }
      continue;
    } catch (const std::exception& ex) {
      XLOG_EVERY_MS(ERR, 5000)
          << fmt::format("Failed to get next oper delta: {}", ex.what());
      lastUpdateSeqNum = 0;
      continue;
    }
    // SwSwitch can send empty operdelta when cancelling the service on
    // shutdown
    if (operSyncRunning_.load() &&
        stateOperDelta.operDelta()->changes()->size()) {
      if (*stateOperDelta.isFullState()) {
        XLOG(DBG2) << "Received full state oper delta from swswitch";
        lastUpdateResult = processFullOperDelta(
            *stateOperDelta.operDelta(), *stateOperDelta.hwWriteBehavior());
      } else {
        auto oldState = hw_->getProgrammedState();
        lastUpdateResult = stateOperDelta.transaction().value()
            ? hw_->stateChangedTransaction(
                  *stateOperDelta.operDelta(),
                  HwWriteBehaviorRAII(*stateOperDelta.hwWriteBehavior()))
            : hw_->stateChanged(
                  *stateOperDelta.operDelta(),
                  HwWriteBehaviorRAII(*stateOperDelta.hwWriteBehavior()));
        if (lastUpdateResult.changes()->empty()) {
          hw_->getPlatform()->stateChanged(
              StateDelta(oldState, hw_->getProgrammedState()));
        }
      }

      // If swswitch has transitioned to configured state,
      // then move hwswitch as well
      if ((hw_->getRunState() == SwitchRunState::INITIALIZED) &&
          (utility::getFirstNodeIf(
               hw_->getProgrammedState()->getSwitchSettings())
               ->getSwSwitchRunState() == SwitchRunState::CONFIGURED)) {
        hw_->switchRunStateChanged(SwitchRunState::CONFIGURED);
      }
    }
    lastUpdateSeqNum = *stateOperDelta.seqNum();
  }
}

fsdb::OperDelta OperDeltaSyncer::processFullOperDelta(
    fsdb::OperDelta& operDelta,
    const HwWriteBehavior& hwWriteBehavior) {
  // Enable deep comparison for full oper delta
  DeltaComparison::PolicyRAII policyGuard{DeltaComparison::Policy::DEEP};
  auto fullStateDelta = StateDelta(std::make_shared<SwitchState>(), operDelta);
  auto delta = StateDelta(hw_->getProgrammedState(), fullStateDelta.newState());
  auto appliedState =
      hw_->stateChanged(delta, HwWriteBehaviorRAII(hwWriteBehavior));
  // return empty oper delta to indicate success. If update was not successful,
  // hwswitch would have crashed.
  CHECK(isStateDeltaEmpty(StateDelta(fullStateDelta.newState(), appliedState)));
  hw_->getPlatform()->stateChanged(delta);
  return fsdb::OperDelta{};
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
